#include "sensors.h"
#include <unistd.h>
#include "esp_timer.h"
#include "storage.h"

#include "sensorUltrassonico.h"

struct storage_data_t storage_data;
struct sensors_data_t sensors_data;
struct sensors_t sensors;

uint8_t flagAudio = 0;
uint8_t flagAcc = 0;
uint8_t flagIR = 0;
uint8_t class_index_detection = 0;

// #define SAVE_EEPROM 1

void task_detect(void *pvParameters)
{
    struct ultrasonic_driver_data_t ultra_attributes = ultrasonic_get_attributes();
    struct ultrasonic_driver_t *ultra = ultrasonic_driver_instance();

    for(;;)
    {  
        printf("cm: %d\tticks: %d\ttempo: %d\n", ultra->get_distance(&ultra_attributes), xTaskGetTickCount(), pdTICKS_TO_MS(xTaskGetTickCount()));
       
        vTaskDelay(250 / portTICK_PERIOD_MS);
        
    }
}

static void manager_relay(void *arg)
{
    struct mcp_driver_t *mcp_device = mcp_driver_instance();
    sensors_data.relayOff = 0;

    for (;;)
    {
        if (sensors_data.relayStatus)
        {
            sensors_data.relayOff += 1;
            if (sensors_data.relayOff >= RELE_OFF)
            {
                sensors_data.relayStatus = 0;
                sensors_data.relayOff = 0;
                //alt gpb7
                gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
                gpio_set_level(GPIO_NUM_4, 0);
                // mcp_device->mode_mcp(IODIRB, P7, LOW);
                // mcp_device->write_pin_mcp(GPIOB, P7, LOW);

            }
            //printf("O relé está %s \n", (sensors_data.relayStatus) ? "True" : "False");
        }
        
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    
}

/**
 * @brief task de grenciamento dos dados que serão enviados para o modelo
 * 
 * @param arg 
 */
static void sensors_task_notify_model(void *arg)
{
    while(!flagAudio || !flagIR || !flagAcc)
    {
        vTaskDelay(100/portTICK_PERIOD_MS);
    }

    uint8_t predict;
    
    struct mcp_driver_t *mcp_device = mcp_driver_instance();
    struct serialUart_driver_data_t *serialUart_driver_data = &serialUart_data_driver;

        
    #ifdef SAVE_EEPROM
        struct storage_device_t *storage_eeprom = storage_device_instance();
    #endif
    xTaskCreate(manager_relay, "manager_relay", 2048, NULL, 10, NULL);
    // xTaskCreate(task_detect,"task_detect", 2048, NULL, 10, NULL);

    for (;;)
    {
        if (flagAudio && flagAcc && flagIR)
        {
            if(xSemaphoreTake(sensors_data.xSemaphore_acc, pdMS_TO_TICKS(50)) &&
                xSemaphoreTake(sensors_data.xSemaphore_mic, pdMS_TO_TICKS(50)) &&
                xSemaphoreTake(sensors_data.xSemaphore_ir, pdMS_TO_TICKS(50)))
            {
                
                flagAudio = 0;
                flagAcc = 0;
                flagIR = 0;

                //ESP_LOGI("sensor before detect RAM", "RAM left %d B\n", esp_get_free_heap_size());
                /* ESP_LOGI("TAG", "Tempo a cada predição: %lld us",
                    esp_timer_get_time() - tempoPred);
                tempoPred = esp_timer_get_time() */;
                //printf("\n");
                
                /* int64_t tempo = esp_timer_get_time(); */

                detect(sensors_data.sensor_ir, &sensors_data.sensor_acc, sensors_data.sensor_audio, &predict, &class_index_detection);
                // printf("Detect: %d\n",heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
                /* ESP_LOGI("TAG", "Tempo de detecção: %lld us",
                    esp_timer_get_time() - tempo); */

                //Predicao instantanea
                serialUart_driver_data->predict_model = 1 << predict;
                //historico de predicoes
                serialUart_driver_data->predict_model_history |= 1 << predict;

                sensors_data.relay = detect_policies();
                
                
                #ifndef TESTE_I2S
                if (sensors_data.relay == HIGH)
                {
                    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
                    gpio_set_level(GPIO_NUM_4, 1);
                    // mcp_device->mode_mcp(IODIRB, P7, LOW);
                    // mcp_device->write_pin_mcp(GPIOB, P7, HIGH);
                    sensors_data.relayOff = 0;
                    sensors_data.relayStatus = sensors_data.relay;

                    #ifdef SAVE_EEPROM
                        //LOG NA EEPROM SOBRE O ATAQUE
                        uint32_t log_data[4], checksum;
                        uint8_t id_flag_log;
                        time_t tt = time(NULL);
                        if (tt<1704067200) //1 jan 2024 00:00 GMT
                            tt = 0;
                        
                        if (storage_data.flag_is_inverted)  id_flag_log = flag_inverter_relay;
                        if (!storage_data.flag_is_inverted) id_flag_log = flag_default_relay;
                        
                        if (class_index_detection == 2) class_index_detection = 4; //0100 (Vibracao)
                        if (class_index_detection == 1) class_index_detection = 2; //0010 (Termico)
                        if (class_index_detection == 0) class_index_detection = 1; //0001 (Normal)
                        checksum = (uint32_t) tt + (uint32_t) id_flag_log + (uint32_t) class_index_detection; 

                        log_data[0] = id_flag_log;
                        log_data[1] = tt;
                        log_data[2] = class_index_detection;
                        log_data[3] = checksum;
                        
                        xQueueSend(storage_data.save_logs_queue, log_data, 0);

                    #endif
                }
                #endif


                xSemaphoreGive(sensors_data.xSemaphore_acc);
                xSemaphoreGive(sensors_data.xSemaphore_mic);
                xSemaphoreGive(sensors_data.xSemaphore_ir);
            }
            else
            {                
                flagAudio = 0;
                flagAcc = 0;
                flagIR = 0;

                xSemaphoreGive(sensors_data.xSemaphore_acc);
                xSemaphoreGive(sensors_data.xSemaphore_mic);
                xSemaphoreGive(sensors_data.xSemaphore_ir);
            }
        }

        vTaskDelay(50/portTICK_PERIOD_MS);
    }
    
}

/**
 * @brief inicialização dos sensores e dos semaforos. 
 * 
 * @return esp_err_t 
 */
esp_err_t sensors_init(void)
{    
    esp_err_t err_sensors;
    struct mic_driver_t *mic_device = mic_driver_instance();

    #ifndef TESTE_I2S
        struct acc_driver_t *acc_device = acc_driver_instance();
        struct infrareds_driver_t *ir_device = infrareds_driver_instance();
        struct mcp_driver_t *mcp_device = mcp_driver_instance();
        struct ultrasonic_driver_data_t ultra_attributes = ultrasonic_get_attributes();
        struct ultrasonic_driver_t *ultra = ultrasonic_driver_instance();
    #endif
    
    err_sensors = mic_device->mic_init();
    if(err_sensors != ESP_OK)
    {
        ESP_LOGE("SENSOR", "Falha ao inicializar o mic");
        return ESP_FAIL;
    }
    
    #ifndef TESTE_I2S

        err_sensors = acc_device->acc_init();
        if(err_sensors != ESP_OK)
        {
            ESP_LOGE("SENSOR", "Falha ao inicializar o acell");
            return ESP_FAIL;
        }
        
        err_sensors = ir_device-> infrareds_init();
        if(err_sensors != ESP_OK)
        {
            ESP_LOGE("SENSOR", "Falha ao inicializar o IR");
            return ESP_FAIL;
        }

        err_sensors = mcp_device-> mcp_driver_init();
        while(err_sensors != ESP_OK)
        {
            ESP_LOGE("SENSOR", "Falha ao inicializar o MCP");
            return ESP_FAIL;
        } 
    
    #endif

    sensors_data.xSemaphore_acc = xSemaphoreCreateBinary();
    sensors_data.xSemaphore_mic = xSemaphoreCreateBinary();
    sensors_data.xSemaphore_ir = xSemaphoreCreateBinary();

    xSemaphoreGive(sensors_data.xSemaphore_acc);
    xSemaphoreGive(sensors_data.xSemaphore_mic);
    xSemaphoreGive(sensors_data.xSemaphore_ir);
    
    #ifdef SAVE_EEPROM
        struct storage_device_t *storage_eeprom = storage_device_instance();

        err_sensors = storage_eeprom->storage_init();
        if(err_sensors != ESP_OK)
        {
            ESP_LOGE("SENSOR", "Falha ao inicializar a eeprom");
            return ESP_FAIL;
        }
        
        
    #endif

    #ifndef TESTE_I2S

        err_sensors = ultra->init(&ultra_attributes);
        if(err_sensors != ULTRASONIC_SUCCESS)
        {
            ESP_LOGE("SENSOR", "Falha ao inicializar o Ultrassonico");
            return ESP_FAIL;
        }

    #endif

    
    
    xTaskCreate(sensors_task_notify_model, "sensors_task_notify_model", 4096, NULL, 10, &sensors_data.xHandle_notify_model);

    

    struct serialUart_driver_t *serialUart = serialUart_driver_instance();
    serialUart->serial_uart_init();

    return ESP_OK;
}

/**
 * @brief atualização dos dados do acelerômetro
 * 
 * @param data_x dado do eixo x
 * @param data_y dado do eixo y
 * @param data_z dado do eixo z
 * @return esp_err_t 
 */
esp_err_t sensors_refresh_acc(float *data_x, float *data_y, float *data_z)
{
    if (xSemaphoreTake(sensors_data.xSemaphore_acc, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        for (size_t i = 0; i < 50; i++)
        {
            sensors_data.sensor_acc[i][0] = data_x[i];
            sensors_data.sensor_acc[i][1] = data_y[i];
            sensors_data.sensor_acc[i][2] = data_z[i];
        }
        flagAcc = 1;
        xSemaphoreGive(sensors_data.xSemaphore_acc);
    }

    
    return ESP_OK;
}

/**
 * @brief atualização dos dados do mic
 * 
 * @param data_audio dados do microfone
 * @return esp_err_t 
 */
esp_err_t sensors_refresh_audio(float *data_audio)
{
    if (xSemaphoreTake(sensors_data.xSemaphore_mic, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        sensors_data.sensor_audio = data_audio;
        flagAudio = 1;
        xSemaphoreGive(sensors_data.xSemaphore_mic);
    }
    
    return ESP_OK;
}

/**
 * @brief atualização dos dados infrared
 * 
 * @param data_ir dados do infrared
 * @return esp_err_t 
 */
esp_err_t sensors_refresh_ir(uint8_t *data_ir)
{
    if (xSemaphoreTake(sensors_data.xSemaphore_ir, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        for (size_t i = 0; i < 50; i++)
        {
            sensors_data.sensor_ir[i] = (float)data_ir[i];
        }
        flagIR = 1;
        xSemaphoreGive(sensors_data.xSemaphore_ir);
    }
    
    return ESP_OK;
}

esp_err_t sensors_autoteste()
{
    struct infrareds_driver_t *ir_device = infrareds_driver_instance();
    struct acc_driver_t *acc_device = acc_driver_instance();
    struct mic_driver_t *mic_device = mic_driver_instance();
    struct serialUart_driver_data_t *serialUart_driver_data = &serialUart_data_driver;

    printf("*AUTOTESTE START\n");
    vTaskSuspend(sensors_data.xHandle_notify_model);
    
    
    esp_err_t autoteste_result;
    autoteste_result = ir_device->infrareds_autoteste(serialUart_driver_data->res_autoteste);
    if (autoteste_result != ESP_OK)
    {
        printf("Autoteste IR Fail \n");
    }
    
    autoteste_result = acc_device->acc_autoteste();
    if (autoteste_result != ESP_OK)
    {
        serialUart_driver_data->res_autoteste[6] = 1;
        printf("Autoteste ACC Fail \n");
    }

    autoteste_result = mic_device->mic_autoteste();
    if (autoteste_result != ESP_OK)
    {
        serialUart_driver_data->res_autoteste[7] = 1;
        printf("Autoteste MIC Fail \n");
    }

    printf("*AUTOTESTE STOP\n");
    vTaskDelay(500/portTICK_PERIOD_MS);
    vTaskResume(sensors_data.xHandle_notify_model);

    return ESP_OK;
}

struct sensors_t *sensors_instance()
{
    if (sensors.sensors_init == 0)
    {
        sensors = (struct sensors_t)
        {
            .sensors_init = &sensors_init,
            .sensors_refresh_acc = &sensors_refresh_acc,
            .sensors_refresh_audio = &sensors_refresh_audio,
            .sensors_refresh_ir = &sensors_refresh_ir,
            .sensors_autoteste = &sensors_autoteste,
        };
    }
    return &sensors;    
}