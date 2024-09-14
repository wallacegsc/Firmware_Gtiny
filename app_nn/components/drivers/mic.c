#include "mic.h"
#include "esp_attr.h"
#include <unistd.h>
#include "esp_timer.h"

#include <stdio.h>
#include <math.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s.h"
#include "esp_system.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

// #define TESTE_I2S 1

#ifdef TESTE_I2S
    #define PIN_NUM_MISO GPIO_NUM_25
    #define PIN_NUM_MOSI GPIO_NUM_15
    #define PIN_NUM_CLK  GPIO_NUM_23
    #define PIN_NUM_CS   GPIO_NUM_27
    static const char *TAG = "GUARDIAN";
    #define MOUNT_POINT "/sdcard"
    #define SPI_DMA_CHAN    1
    #define NUM_BUFFER_RECORDED 20
    #define NUM_SAMPLES_RECORDED 16000
#endif

struct mic_driver_data_t mic_driver_data;
struct mic_driver_t mic_driver;

float IRAM_ATTR m_audioBuffer1[16000];
//float m_audioBuffer2[16000];

/**
 * @brief Retorna os métodos do controlador i2s
 * 
 * @return struct i2s_controller_t* 
 */
struct mic_driver_data_t *mic_get_methods_i2s()
{
    return mic_driver_data.mic_i2s_methods;
}

struct mic_driver_data_t *mic_get_data_i2s()
{
    return mic_driver_data.mic_i2s_data;
}


/**
 * @brief Configura os parâmetros do microfone
 * 
 * @return esp_err_t 
 */
esp_err_t i2s_configure()
{
    struct i2s_controller_t *mic_i2s_methods = mic_get_methods_i2s();
    struct i2s_controller_data_t *sampler = mic_get_data_i2s();
    
    //Fixes for SPH0645
    REG_SET_BIT(I2S_TIMING_REG(mic_i2s_methods->getI2SPort()), BIT(9));
    REG_SET_BIT(I2S_TIMING_REG(mic_i2s_methods->getI2SPort()), BIT(9));
    REG_SET_BIT(I2S_CONF_REG(mic_i2s_methods->getI2SPort()), I2S_RX_MSB_SHIFT);

    esp_err_t err_i2s = i2s_set_pin(mic_i2s_methods->getI2SPort(), &(sampler->m_i2sPins));
    if(err_i2s != ESP_OK)
    {
        ESP_LOGE("I2S", "Erro de configuração do I2S");
        return ESP_FAIL;
    }

    return ESP_OK;
}
/**
 * @brief task de captura do audio
 * 
 * @param param 
 */
void i2sReaderTask(void *param)
{
    struct i2s_controller_data_t *sampler = mic_get_data_i2s();
    struct i2s_controller_t *sampler_methods = mic_get_methods_i2s();

    while (true)
    {     
        // wait for some data to arrive on the queue
        i2s_event_t evt;
        if (xQueueReceive(sampler->m_i2sQueue, &evt, portMAX_DELAY) == pdPASS)
        {
            if (evt.type == I2S_EVENT_RX_DONE)
            {
                size_t bytesRead = 0;
                do
                {
                    // read data from the I2S peripheral
                    uint8_t i2sData[1024];
                    // read from i2s
                    i2s_read(sampler_methods->getI2SPort(), i2sData, 1024, &bytesRead, 10);
                    // process the raw data
                    sampler_methods->processI2SData(i2sData, bytesRead);
                    
                } while (bytesRead > 0);
            }
        }
    }
}

/**
 * @brief 
 * 
 * @param i2sPort 
 * @param i2sConfig 
 * @param bufferSizeInBytes 
 * @param writerTaskHandle 
 * @return esp_err_t 
 */
esp_err_t i2s_start_mic(i2s_port_t i2sPort, i2s_config_t i2sConfig, int32_t bufferSizeInBytes, TaskHandle_t writerTaskHandle)
{
    struct i2s_controller_t *mic_i2s_methods = mic_get_methods_i2s();
    struct i2s_controller_data_t *mic_i2s_data = mic_get_data_i2s();

    mic_i2s_data->m_i2sPort = i2sPort;
    mic_i2s_data->m_writerTaskHandle = writerTaskHandle;
    mic_i2s_data->m_bufferSizeInSamples = bufferSizeInBytes / sizeof(int32_t);
    mic_i2s_data->m_bufferSizeInBytes = bufferSizeInBytes;

    mic_i2s_data->m_audioBuffer2 = (float *)malloc(bufferSizeInBytes);

    mic_i2s_data->m_currentAudioBuffer = m_audioBuffer1;
    mic_i2s_data->m_capturedAudioBuffer = mic_i2s_data->m_audioBuffer2;

    mic_i2s_data->m_writerTaskHandle = writerTaskHandle;

    esp_err_t err_mic = i2s_driver_install(mic_i2s_data->m_i2sPort, &i2sConfig, 4, &(mic_i2s_data->m_i2sQueue));

    if(err_mic != ESP_OK)
    {
        ESP_LOGE("I2S", "Erro de instalação  no driver");
        return ESP_FAIL;
    }

    i2s_configure();


    TaskHandle_t readerTaskHandle;
    xTaskCreatePinnedToCore(i2sReaderTask, "i2s Reader Task", 1*2048, mic_i2s_data, 1, &readerTaskHandle, 0);

    return ESP_OK;

}

void i2sMemsWriterTask(void *param)
{
    //struct i2s_controller_data_t *sampler = (struct i2s_controller_data_t *)param;
    struct i2s_controller_t *sampler_methods = mic_get_methods_i2s();
    struct i2s_controller_data_t *sampler = mic_get_data_i2s();
    struct sensors_t *sensors_methods = sensors_instance();

#ifdef TESTE_I2S
   int num_interations_to_save = 1;

    ESP_LOGI(TAG, "Using SPI peripheral");

     esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
    ESP_LOGI(TAG, "Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = SPI2_HOST;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");
    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(TAG, "Opening file");
    FILE* f = fopen("/sdcard/i2s.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
#endif



    struct i2s_controller_data_t *mic_i2s_data = mic_get_data_i2s();
   
    

    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(100);  
    setup();
    
    while (true)
    {        
        // wait for some samples to save
        uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
        if (ulNotificationValue > 0)
        {  
            float *audio = sampler_methods->getCapturedAudioBuffer();
            
            sensors_methods->sensors_refresh_audio(audio);
            ulNotificationValue = 0;

            #ifdef TESTE_I2S
                
                if(NUM_BUFFER_RECORDED == 1)
                {
                    fclose(f);
                    ESP_LOGI(TAG, "File written");
                    esp_vfs_fat_sdmmc_unmount();
                    ESP_LOGI(TAG, "Card unmounted"); 
                    num_interations_to_save++;  
                }

                else if(num_interations_to_save == (NUM_BUFFER_RECORDED))
                {
                    fclose(f);
                    ESP_LOGI(TAG, "File written");
                    esp_vfs_fat_sdmmc_unmount();
                    ESP_LOGI(TAG, "Card unmounted");  
                    num_interations_to_save++; 
                }

                else if(num_interations_to_save < (NUM_BUFFER_RECORDED))
                {
                    fwrite((uint8_t *)audio, 1,  mic_i2s_data->m_bufferSizeInBytes, f);
                    num_interations_to_save++;
                }

                
            #endif            
    
            
        }
        
        vTaskDelay(250/portTICK_PERIOD_MS);
  }
  
}

esp_err_t mic_autoteste(void)
{
    struct i2s_controller_t *sampler_methods = mic_get_methods_i2s();
    float *p = sampler_methods->getCapturedAudioBuffer();
    float value[20];
    memcpy(value, &p[15980], 80);
    printf("Autoteste MIC \n");
    for (size_t i = 0; i < 20; i++)
    {
        if (value[i] == 0.0)
        {
            printf("value refrence mic pos %d value -> %f \n", i, value[i]);
            return ESP_FAIL;
        }
    }
    
    return ESP_OK;
}

/**
 * @brief Função de inicialização do microphone sph04
 * 
 * @return esp_err_t 
 */
esp_err_t mic_init()
{    
    mic_driver_data.mic_i2s_methods = i2s_controller_instance();
    mic_driver_data.mic_i2s_data = &i2s_data_controller;

    struct i2s_controller_data_t *sampler = mic_get_data_i2s();
    struct i2s_controller_t *mic_i2s_methods = mic_get_methods_i2s();
    
    // i2s config for reading from both channels of I2S
    i2s_config_t i2sMemsConfigBothChannels = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = 0x01,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0};


    i2s_pin_config_t i2sPins = {
        .bck_io_num = GPIO_NUM_26,              // *microphone BCLK    
        .ws_io_num = GPIO_NUM_27,               // *microphone LRCL
        .data_out_num = I2S_PIN_NO_CHANGE,      // *microphone not connected  
        .data_in_num = GPIO_NUM_25};            // *microphone DOUT

    sampler->m_i2sPins = i2sPins; 

    mic_i2s_methods->i2s_init();   

    TaskHandle_t i2sMemsWriterTaskHandle;
    xTaskCreatePinnedToCore(i2sMemsWriterTask, "I2S Writer Task", 3*4096, sampler, 1, &i2sMemsWriterTaskHandle, 1);
    i2s_start_mic(I2S_NUM_1,i2sMemsConfigBothChannels, 4*NUM_SAMPLES_RECORDED, i2sMemsWriterTaskHandle);
    
    return ESP_OK;
}

struct mic_driver_t *mic_driver_instance()
{
    if (mic_driver.mic_init == 0)
    {
        mic_driver = (struct mic_driver_t)
        {
            .mic_init = &mic_init,
            .i2s_configure = &i2s_configure,
            .i2s_start_mic = &i2s_start_mic,
            .mic_autoteste = &mic_autoteste,
        };
    }
    return &mic_driver;
}

