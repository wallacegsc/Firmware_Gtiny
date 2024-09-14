#ifndef SENSORS_H_
#define SENSORS_H_
#include <stdint.h>

#include "acc.h"
#include "mic.h"
#include "infrareds.h"
#include "mcp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "serialUart.h"

#define RELE_OFF 3

struct sensors_data_t
{
    volatile float *sensor_audio;
    volatile float sensor_acc[50][3];
    volatile float sensor_ir[50];

    uint8_t relay, relayStatus, relayOff;

    SemaphoreHandle_t xSemaphore_acc;
    SemaphoreHandle_t xSemaphore_mic;
    SemaphoreHandle_t xSemaphore_ir;

    TaskHandle_t xHandle_notify_model;
};

// typos de funcoes do device_driver
typedef esp_err_t (*sensors_init_t)();
typedef esp_err_t (*sensors_refresh_acc_t)(float *data_x, float *data_y, float *data_z);
typedef esp_err_t (*sensors_refresh_audio_t)(float *data_audio);
typedef esp_err_t (*sensors_refresh_ir_t)(uint8_t *data_ir);
typedef esp_err_t (*sensors_autoteste_t)();

struct sensors_t
{
    sensors_init_t sensors_init;
    sensors_refresh_acc_t sensors_refresh_acc;
    sensors_refresh_audio_t sensors_refresh_audio;
    sensors_refresh_ir_t sensors_refresh_ir;
    sensors_autoteste_t sensors_autoteste;
};

struct sensors_t *sensors_instance();

#endif
