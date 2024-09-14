#ifndef I2S_CONTROLLER_H_
#define I2S_CONTROLLER_H_

#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "esp_system.h"
#include "esp_attr.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/i2s.h"
#include "soc/i2s_reg.h"
#include "esp_log.h"
#include "esp_err.h"

#include <sys/unistd.h>
#include <sys/stat.h>


struct i2s_controller_data_t
{
    //float m_audioBuffer1[16000];
    //float *m_audioBuffer1;
    
    float *m_audioBuffer2;
    int32_t m_audioBufferPos;

    float *m_currentAudioBuffer;
    
    //buffer containing samples that have been captured already
    float *m_capturedAudioBuffer;

    //size of the audio buffers in bytes
    int32_t m_bufferSizeInBytes;

    //size of the audio buffers in samples
    int32_t m_bufferSizeInSamples;

    //I2S reader task
    TaskHandle_t m_readerTaskHandle;

    //writer task
    TaskHandle_t m_writerTaskHandle;

    //i2s reader queue
    QueueHandle_t m_i2sQueue;

    //i2s pins
    i2s_pin_config_t m_i2sPins;

    //i2s port
    i2s_port_t m_i2sPort;

};

typedef esp_err_t (*i2s_init_t)(void);
typedef esp_err_t (*i2s_add_sample_t)(float sample);
typedef i2s_port_t (*getI2SPort_t)(void);
typedef esp_err_t (*processI2SData_t)(uint8_t *i2sData, size_t bytesRead);
typedef float* (*getCapturedAudioBuffer_t)(void);

struct i2s_controller_t
{
    i2s_init_t i2s_init;
    i2s_add_sample_t i2s_add_sample;
    getI2SPort_t getI2SPort;
    processI2SData_t processI2SData;
    getCapturedAudioBuffer_t getCapturedAudioBuffer;
};

struct i2s_controller_t *i2s_controller_instance();
struct i2s_controller_data_t i2s_data_controller;

#endif