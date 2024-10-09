#ifndef MIC_DRIVER_H_
#define MIC_DRIVER_H_

#include "i2s_controller.h"
#include "sensors.h"
#include "main_functions.h"

#define NUM_SAMPLES_RECORDED 16000
#define SDCARD_CS_PIN GPIO_NUM_15
struct mic_driver_data_t
{
    struct i2s_controller_data_t *mic_i2s_data;
    struct i2s_controller_t *mic_i2s_methods;
};

typedef esp_err_t (*mic_init_t)(void);
typedef esp_err_t (*i2s_configure_t)(void);
typedef esp_err_t (*i2s_start_t)(i2s_port_t i2sPort, i2s_config_t i2sConfig, 
                            int32_t bufferSizeInBytes, TaskHandle_t writerTaskHandle);
typedef esp_err_t (*mic_autoteste_t)(void);
void i2sReaderTask(void *param);
void i2sMemsWriterTask(void *param);

struct mic_driver_t
{
    mic_init_t mic_init;
    i2s_configure_t i2s_configure;
    i2s_start_t i2s_start_mic;
    mic_autoteste_t mic_autoteste;
};

struct mic_driver_t *mic_driver_instance();

#endif