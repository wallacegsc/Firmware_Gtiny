
/*
 * mcp_23018.c
 *
 *  Created on: May 20, 2022
 *      Author: Maysa Jhiovanna
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c.h"
#include "mcp.h"
#include "esp_log.h"
#include "esp_err.h"
#include "time.h"
#include "storage.h"

#define LOG_CONNECT 1

struct mcp_driver_t mcp_driver;
struct storage_data_t storage_data;
QueueHandle_t mcp_irq_evt_queue;


void reset_mcp()
{
    gpio_pad_select_gpio(GPIO_NUM_26);
    gpio_set_direction(GPIO_NUM_26, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_26, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_26, 0);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_26, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
}

static void IRAM_ATTR mcp_esp_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(mcp_irq_evt_queue, &gpio_num, NULL);
}

static void mcp_task_notify_irq(void *arg)
{
    struct mcp_driver_t *mcp_driver = mcp_driver_instance();
    
    #ifdef SAVE_EEPROM
        struct storage_device_t *storage_eeprom = storage_device_instance();
    #endif
    
    uint32_t io_num;

    uint8_t registerGpioA[8];
    uint8_t registerGpioAState[8];
    uint8_t read;
    //mcp_driver->read_pin_mcp(GPIOA, P0, &readMcp);
    mcp_driver->read_group_mcp(GPIOA, &read);
    for (size_t i = 0; i < 8; i++)
    {
        registerGpioA[i] = (read >> i) & 0x01;
        registerGpioAState[i] = (read >> i) & 0x01;
    }
    
    for (;;)
    {
        mcp_driver->read_group_mcp(GPIOA, &read);
        if (xQueueReceive(mcp_irq_evt_queue, &io_num, pdMS_TO_TICKS(10)))
        {
           
            //mcp_driver->read_group_mcp(INTCAPA, &read);
            mcp_driver->read_group_mcp(GPIOA, &read);
         
            for (size_t i = 0; i < 8; i++)
            {
                registerGpioA[i] = (read >> i) & 0x01;
            }
        
            //DET_CASE
            if (registerGpioAState[0] != registerGpioA[0])
            {
                registerGpioAState[0] = registerGpioA[0];
            }

            //DET_GPIO_EM_1
            if (registerGpioAState[1] != registerGpioA[1])
            {
                registerGpioAState[1] = registerGpioA[1];
            }
            
            //DET_GPIO_EM_2
            if (registerGpioAState[2] != registerGpioA[2])
            {
                registerGpioAState[2] = registerGpioA[2];
            }

            //DET_GPIO_EM_3
            if (registerGpioAState[3] != registerGpioA[3])
            {
                registerGpioAState[3] = registerGpioA[3];
            }

            //DET_GPIO_EM_5
            if (registerGpioAState[4] != registerGpioA[4])
            {
                registerGpioAState[4] = registerGpioA[4];
            }

            //DET_GPIO_EM_4
            if (registerGpioAState[5] != registerGpioA[5])
            {
                registerGpioAState[5] = registerGpioA[5];
            }

            //DET_GPIO_EM_6
            if (registerGpioAState[6] != registerGpioA[6])
            {
                registerGpioAState[6] = registerGpioA[6];
            }

            //DET_ENERGIA
            if (registerGpioAState[7] != registerGpioA[7])
            {
                registerGpioAState[7] = registerGpioA[7];
            }

            
            #ifdef SAVE_EEPROM
            // case baixo 
            // p0 p7 e p4
            // if (registerGpioA[0] == 1 )//|| registerGpioA[4] == 1
            // {
                printf("event \n");
                //LOG NA EEPROM SOBRE O EVENTO
                uint32_t log_data[4], checksum;
                uint8_t id_flag_log;
                time_t tt = time(NULL);
                if (tt<1704067200) //1 jan 2024 00:00 GMT
                    tt = 0;

                if (storage_data.flag_is_inverted)  id_flag_log = flag_inverter_event;
                if (!storage_data.flag_is_inverted) id_flag_log = flag_default_event;
                    
                uint8_t req = 0;
                for (size_t i = 0; i < 7; i++)
                {
                    req |= (!registerGpioA[i]) << i;
                }
                req |= registerGpioA[7] << 7;
                
                checksum = (uint32_t) tt + (uint32_t) id_flag_log + (uint32_t) req;
                
                log_data[0] = id_flag_log;
                log_data[1] = tt;
                log_data[2] = req;
                log_data[3] = checksum;

                xQueueSend(storage_data.save_logs_queue, log_data, 0);
            }
                
            #endif
        }
    // }
}

int mcp_register_irq(void)
{
    // código para cadastrar a irq para as GPIOs desejadas
    gpio_config_t io_conf = {0};
    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    // bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_39);
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    
    // create a queue to handle gpio event from isr
    mcp_irq_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // start gpio task
    xTaskCreate(mcp_task_notify_irq, "mcp_task_notify_irq", 2048, NULL, 10, NULL);


    esp_err_t err;
    err = gpio_isr_handler_add(GPIO_NUM_39, mcp_esp_isr_handler, (void *)GPIO_NUM_39);

    if(err != ESP_OK){
        ESP_LOGE("MCP", "Interrupção mal configurada");
        return ESP_FAIL;
    }


    ESP_LOGI("MCP", "interrupção configurada com sucesso");
    return 0;
}

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t register_interrupt(void){
    struct i2c_controller_t *i2c_controller = i2c_controller_instance();
    struct mcp_driver_t *mcp_driver = mcp_driver_instance();
    
    uint8_t read_IOCON;
    uint8_t receive;
    uint8_t receivegpa;

    //Interrupt DET_ENG
    mcp_driver->mode_mcp(IODIRA, P7,INPUT);
    mcp_driver->write_pin_mcp(GPINTENA, P7, HIGH);
    mcp_driver->write_pin_mcp(INTCONA, P7, HIGH);
    mcp_driver->write_pin_mcp(DEFVALA, P7, LOW);
    

    //Interrupt GPIO_CASE
    mcp_driver->mode_mcp(IODIRA, P0, INPUT);
    mcp_driver->write_pin_mcp(GPINTENA, P0, HIGH);
    mcp_driver->write_pin_mcp(INTCONA, P0, HIGH);
    mcp_driver->write_pin_mcp(DEFVALA, P0, LOW);
    
    #ifndef PLACA_CONCENTRADORA
        printf("Placa distribuidora \n");
        //Interrupt DET_GPIO6
        mcp_driver->mode_mcp(IODIRA, P6,INPUT);
        mcp_driver->write_pin_mcp(GPINTENA, P6, HIGH);
        mcp_driver->write_pin_mcp(INTCONA, P6, LOW);
        mcp_driver->write_pin_mcp(DEFVALA, P6, LOW);
        mcp_driver->write_pin_mcp(GPPUA, P6, HIGH);

        //Interrupt DET_GPIO4
        mcp_driver->mode_mcp(IODIRA, P5,INPUT);
        mcp_driver->write_pin_mcp(GPINTENA, P5, HIGH);
        mcp_driver->write_pin_mcp(INTCONA, P5, LOW);
        mcp_driver->write_pin_mcp(DEFVALA, P5, LOW);
        mcp_driver->write_pin_mcp(GPPUA, P5, HIGH);

        //Interrupt DET_GPIO5
        mcp_driver->mode_mcp(IODIRA, P4,INPUT);
        mcp_driver->write_pin_mcp(GPINTENA, P4, HIGH);
        mcp_driver->write_pin_mcp(INTCONA, P4, LOW);
        mcp_driver->write_pin_mcp(DEFVALA, P4, LOW);
        mcp_driver->write_pin_mcp(GPPUA, P4, HIGH);

        //Interrupt DET_GPIO3
        mcp_driver->mode_mcp(IODIRA, P3,INPUT);
        mcp_driver->write_pin_mcp(GPINTENA, P3, HIGH);
        mcp_driver->write_pin_mcp(INTCONA, P3, LOW);
        mcp_driver->write_pin_mcp(DEFVALA, P3, LOW);
        mcp_driver->write_pin_mcp(GPPUA, P3, HIGH);

        //Interrupt DET_GPIO2
        mcp_driver->mode_mcp(IODIRA, P2,INPUT);
        mcp_driver->write_pin_mcp(GPINTENA, P2, HIGH);
        mcp_driver->write_pin_mcp(INTCONA, P2, LOW);
        mcp_driver->write_pin_mcp(DEFVALA, P2, LOW);
        mcp_driver->write_pin_mcp(GPPUA, P2, HIGH);

        //Interrupt DET_GPIO1
        mcp_driver->mode_mcp(IODIRA, P1,INPUT);
        mcp_driver->write_pin_mcp(GPINTENA, P1, HIGH);
        mcp_driver->write_pin_mcp(INTCONA, P1, LOW);
        mcp_driver->write_pin_mcp(DEFVALA, P1, LOW);
        mcp_driver->write_pin_mcp(GPPUA, P1, HIGH);

    #else
        printf("Placa concentradora \n");
        mcp_driver->mode_mcp(IODIRA, P4,INPUT);
        mcp_driver->write_pin_mcp(GPINTENA, P4, HIGH);
        mcp_driver->write_pin_mcp(INTCONA, P4, HIGH);
        mcp_driver->write_pin_mcp(DEFVALA, P4, LOW);
        mcp_driver->write_pin_mcp(GPPUA, P4, HIGH);

    #endif

    if(i2c_controller->i2c_read_8b(MCP_I2C_MACHINE, MCP_I2C_ADDRESS, IOCONA, &read_IOCON) != ESP_OK) return ESP_FAIL;

    printf("104\n");
    if(read_IOCON == 0){
        i2c_controller->i2c_read_8b(MCP_I2C_MACHINE, MCP_I2C_ADDRESS, INTCAPA, &receive);
        printf("Lendo valor em INTCAPA: 0x%X\n", receive);
    }

    else{
        i2c_controller->i2c_read_8b(MCP_I2C_MACHINE, MCP_I2C_ADDRESS, GPIOA, &receivegpa);
        printf("Lendo valor em GPIOA: 0x%X\n", receivegpa);
    }

    return ESP_OK;
}


esp_err_t mcp_driver_init(void){
  struct i2c_controller_t *i2c_controller = i2c_controller_instance();

    uint8_t mcp_address = NULL;
    if(i2c_controller->i2c_read_8b_mcp(MCP_I2C_MACHINE, MCP_I2C_ADDRESS, IOCONA, &mcp_address) != ESP_OK)
    {
        ESP_LOGE("MCP23018", "Falha na comunicação");
        return ESP_FAIL;
    }
    if(mcp_address != ALL_SET)
    {
        ESP_LOGE("MCP23018", "Dispositivo não encontrado");
        return ESP_FAIL;
    }
    ESP_LOGI("MCP23018", "Dispositivo encontrado");
    //reset_mcp();
    //mcp_driver.write_pin_mcp(GPIOB, ALL_RESET, false);
    ESP_LOGI("MCP23018", "Dispositivo resetado");
    
    //mcp_driver.mode_mcp(IODIRB, P7, LOW);
    mcp_driver.write_group_mcp(IODIRB, 0x00);
    mcp_driver.write_group_mcp(GPIOB, 0x00);

    ESP_LOGI("MCP23018", "Rele desligado por padrao");

    if(register_interrupt() != ESP_OK){
        ESP_LOGE("MCP23018", "Interrupção não configurada");
        return ESP_FAIL;
    }

    mcp_register_irq();


    return ESP_OK;
}

esp_err_t write_pin_mcp(uint8_t port, uint8_t pin, uint8_t value)
{  
    struct i2c_controller_t *i2c_controller = i2c_controller_instance();
    uint8_t read_value;
    vTaskDelay(10/portTICK_PERIOD_MS);
    if (pin != ALL_RESET)
    {
        i2c_controller->i2c_read_8b_mcp(MCP_I2C_MACHINE, MCP_I2C_ADDRESS, port, &read_value);
        if(value) read_value |= pin;
        else read_value &= ~pin;
        if (i2c_controller->i2c_write_mcp(MCP_I2C_MACHINE, MCP_I2C_ADDRESS, port, read_value) != ESP_OK)
        {
            ESP_LOGE("MCP23017", "Falha ao configurar o parâmetro selecionado");
            return ESP_FAIL;
        }
    }
    else if (pin == ALL_RESET)
    {
        if (i2c_controller->i2c_write_mcp(MCP_I2C_MACHINE, MCP_I2C_ADDRESS, port, ALL_RESET) != ESP_OK)
        {
            ESP_LOGE("MCP23017", "Falha ao configurar o parâmetro selecionado");
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}


esp_err_t write_group_mcp(uint8_t port, uint8_t value)
{
    struct i2c_controller_t *i2c_controller = i2c_controller_instance();

    if (i2c_controller->i2c_write_mcp(MCP_I2C_MACHINE, MCP_I2C_ADDRESS, port, value) != ESP_OK)
       {
        ESP_LOGE("MCP23017", "Falha ao configurar o parâmetro selecionado");
        return ESP_FAIL;
    }

    return ESP_OK;
}


esp_err_t read_pin_mcp(uint8_t port, uint8_t pin, uint8_t *read)
{
    struct i2c_controller_t *i2c_controller = i2c_controller_instance();
    uint8_t read_value;

    if(i2c_controller->i2c_read_8b_mcp(MCP_I2C_MACHINE, MCP_I2C_ADDRESS, port, &read_value) != ESP_OK) return ESP_FAIL;
    if ((read_value & (pin)) == (pin))
    {
        *read = HIGH; 
    }
    else
    {
        *read = LOW;
    }
    
    return ESP_OK;
}

esp_err_t read_group_mcp(uint8_t port, uint8_t *read)
{
    struct i2c_controller_t *i2c_controller = i2c_controller_instance();

    if(i2c_controller->i2c_read_8b_mcp(MCP_I2C_MACHINE, MCP_I2C_ADDRESS, port, read) != ESP_OK) return ESP_FAIL;
    
    return ESP_OK;
}


struct mcp_driver_t *mcp_driver_instance()
{
    if (mcp_driver.mode_mcp == 0)
    {
        mcp_driver = (struct mcp_driver_t){
            .mcp_driver_init = &mcp_driver_init,
            .mode_mcp = &write_pin_mcp,
            .write_pin_mcp = &write_pin_mcp,
            .write_group_mcp = &write_group_mcp,
            .read_pin_mcp = &read_pin_mcp,
            .read_group_mcp = &read_group_mcp,
            .pullup_pin = &write_pin_mcp,
            // .register_interrupt = &register_interrupt,
        };
    }
    return &mcp_driver;
}