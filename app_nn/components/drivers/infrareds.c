#include "infrareds.h"

struct infrareds_driver_data_t infrareds_driver_data;
struct infrareds_driver_t infrareds_driver;

esp_err_t infrared_config_pwm(void)
{
    ledc_timer_config_t timerInfrared = {
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 1000,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    ledc_channel_config_t channelInfrared = {
        .channel = LEDC_CHANNEL_0,
        .duty = 511,
        .gpio_num = GPIO_NUM_5,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0,
    };

    esp_err_t err = ledc_timer_config(&timerInfrared);
    if (err != ESP_OK)
    {
        ESP_LOGE("IR", "Falha na configuração do timer do PWM");
        return ESP_FAIL;
    }

    err = ledc_channel_config(&channelInfrared);
    if (err != ESP_OK)
    {
        ESP_LOGE("IR", "Falha na configuração do canal do PWM");
        return ESP_FAIL;
    }

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 511);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    
    return ESP_OK;
}

static void infrared_task_recharge_reference(void* arg)
{
    while (true)
    {
        ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
        vTaskDelay(10000/portTICK_PERIOD_MS);
    }
    
}

static void infrared_task_read_data(void* arg)
{
    struct sensors_t *sensors_methods = sensors_instance();   
    while (true)
    {
        for(int i = 0; i < 50; i++)
        {
            infrareds_driver_data.infrared_data[i] = gpio_get_level(GPIO_NUM_38);
            vTaskDelay(20/portTICK_PERIOD_MS);
        }
        
        sensors_methods->sensors_refresh_ir(&infrareds_driver_data.infrared_data);
        
    }
    
}

esp_err_t infrareds_autoteste(uint8_t *res_autoteste)
{
    struct mcp_driver_t *mcp_device = mcp_driver_instance();
    vTaskSuspend(infrareds_driver_data.xHandle_infrared);
    printf("Autoteste IR \n");
    
    #ifndef PLACA_CONCENTRADORA
        for (size_t i = 1; i < 7; i++)
        {
            uint8_t cont = 0;
            uint8_t resultado = 0;
            vTaskDelay(50/portTICK_PERIOD_MS);
            mcp_device->write_pin_mcp(GPIOB, 1 << i, HIGH);
            do
            {
                vTaskDelay(50/portTICK_PERIOD_MS);
                resultado = gpio_get_level(GPIO_NUM_38);
                res_autoteste[i -1] = !resultado;
                printf("Valor do IR%d = %d \n", i, resultado);
                cont++;
            } while (cont < 4 && !resultado);
            mcp_device->write_pin_mcp(GPIOB, 1 << i, LOW);
        }
    #endif

    #ifdef PLACA_CONCENTRADORA

        mcp_device->write_pin_mcp(GPIOA, P5, LOW);
        vTaskDelay(10/portTICK_PERIOD_MS);
        mcp_device->write_pin_mcp(GPIOA, P3, LOW);
        vTaskDelay(10/portTICK_PERIOD_MS);
        mcp_device->write_pin_mcp(GPIOA, P2, LOW);
        vTaskDelay(10/portTICK_PERIOD_MS);
        mcp_device->write_pin_mcp(GPIOA, P1, LOW);
        vTaskDelay(10/portTICK_PERIOD_MS);

        mcp_device->write_pin_mcp(GPIOA, P6, HIGH);
        vTaskDelay(10/portTICK_PERIOD_MS);

        uint8_t cont = 0;
        uint8_t resultado = 0;

        //teste IR 1
        mcp_device->write_pin_mcp(GPIOA, P1, HIGH);
        do
        {
            vTaskDelay(200/portTICK_PERIOD_MS);
            resultado = gpio_get_level(GPIO_NUM_38);
            res_autoteste[0] = !resultado;
            printf("Valor do IR1 = %d \n", resultado);
            cont++;
        } while (cont < 4 && !resultado);
        
        //teste IR 2
        mcp_device->write_pin_mcp(GPIOA, P1, LOW);
        vTaskDelay(10/portTICK_PERIOD_MS);
        mcp_device->write_pin_mcp(GPIOA, P2, HIGH);
        cont = 0;
        resultado = 0;
        
        do
        {
            vTaskDelay(200/portTICK_PERIOD_MS);
            resultado = gpio_get_level(GPIO_NUM_38);
            res_autoteste[1] = !resultado;
            printf("Valor do IR2 = %d \n", resultado);
            cont++;
        } while (cont < 4 && !resultado);

        //teste IR 3
        mcp_device->write_pin_mcp(GPIOA, P2, LOW);
        vTaskDelay(10/portTICK_PERIOD_MS);
        mcp_device->write_pin_mcp(GPIOA, P3, HIGH);
        cont = 0;
        resultado = 0;
        
        do
        {
            vTaskDelay(200/portTICK_PERIOD_MS);
            resultado = gpio_get_level(GPIO_NUM_38);
            res_autoteste[2] = !resultado;
            printf("Valor do IR3 = %d \n", resultado);
            cont++;
        } while (cont < 4 && !resultado);

        //teste IR 4
        mcp_device->write_pin_mcp(GPIOA, P3, LOW);
        vTaskDelay(10/portTICK_PERIOD_MS);
        mcp_device->write_pin_mcp(GPIOA, P5, HIGH);
        cont = 0;
        resultado = 0;
        
        do
        {
            vTaskDelay(200/portTICK_PERIOD_MS);
            resultado = gpio_get_level(GPIO_NUM_38);
            res_autoteste[3] = !resultado;
            printf("Valor do IR4 = %d \n", resultado);
            cont++;
        } while (cont < 4 && !resultado);

        mcp_device->write_pin_mcp(GPIOA, P6, LOW);
        vTaskDelay(10/portTICK_PERIOD_MS);

        //Habilitando os pinos de recepção do IR
        mcp_device->write_pin_mcp(GPIOA, P3, HIGH);
        vTaskDelay(10/portTICK_PERIOD_MS);
        mcp_device->write_pin_mcp(GPIOA, P2, HIGH);
        vTaskDelay(10/portTICK_PERIOD_MS);
        mcp_device->write_pin_mcp(GPIOA, P1, HIGH);
        vTaskDelay(10/portTICK_PERIOD_MS);
        mcp_device->write_pin_mcp(GPIOA, P5, HIGH);
        
    #endif

    vTaskResume(infrareds_driver_data.xHandle_infrared);
    return ESP_OK;
}

esp_err_t infrareds_init(void)
{
    gpio_config_t config_io = {
        .pin_bit_mask = (1ULL << GPIO_NUM_38),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&config_io);
    if (err != ESP_OK)
    {
        ESP_LOGE("IR", "Falha na configuração da gpio do IR");
        return ESP_FAIL;
    }
    
    err = infrared_config_pwm();
    if (err != ESP_OK)
    {
        ESP_LOGE("IR", "Falha na configuração do pwm do infrared");
        return ESP_FAIL;
    }

        #ifdef PLACA_CONCENTRADORA
        struct mcp_driver_t *mcp_device = mcp_driver_instance();

        //disable emitter
        mcp_device->write_pin_mcp(IODIRA, P6, LOW);
        vTaskDelay(10/portTICK_PERIOD_MS);
        mcp_device->write_pin_mcp(GPIOA, P6, LOW);
        vTaskDelay(10/portTICK_PERIOD_MS);
        
        //Habilitando como saída os pinos do mcp para habilitar a rec IRs
        mcp_device->write_pin_mcp(IODIRA, P5, LOW);
        vTaskDelay(10/portTICK_PERIOD_MS);
        mcp_device->write_pin_mcp(IODIRA, P3, LOW);
        vTaskDelay(10/portTICK_PERIOD_MS);
       
        mcp_device->write_pin_mcp(IODIRA, P2, LOW);
        vTaskDelay(10/portTICK_PERIOD_MS);
        mcp_device->write_pin_mcp(IODIRA, P1, LOW);
        vTaskDelay(10/portTICK_PERIOD_MS);
       
        //Habilitando a rec dos IRs
        mcp_device->write_pin_mcp(GPIOA, P5, HIGH);
        vTaskDelay(10/portTICK_PERIOD_MS);
        mcp_device->write_pin_mcp(GPIOA, P3, HIGH);
        vTaskDelay(10/portTICK_PERIOD_MS);
        
        mcp_device->write_pin_mcp(GPIOA, P2, HIGH);
        vTaskDelay(10/portTICK_PERIOD_MS);
        mcp_device->write_pin_mcp(GPIOA, P1, HIGH);

    #endif

    ESP_LOGI("IR", "GPIO do IR configurado com sucesso");
    xTaskCreate(infrared_task_read_data, "infrared_task_read_data", 1024, NULL, 10, &infrareds_driver_data.xHandle_infrared);
    xTaskCreate(infrared_task_recharge_reference, "infrared_task_recharge_refrence", 512, NULL, 10, NULL);

    return ESP_OK;
}

struct infrareds_driver_t *infrareds_driver_instance()
{
    if (infrareds_driver.infrareds_init == 0)
    {
        infrareds_driver = (struct infrareds_driver_t){
            .infrareds_init = &infrareds_init,
            .infrareds_autoteste = &infrareds_autoteste,
        };
    }

    return &infrareds_driver;
};
