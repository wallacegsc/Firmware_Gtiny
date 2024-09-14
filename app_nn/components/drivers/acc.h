#ifndef ACC_DRIVER_H_
#define ACC_DRIVER_H_

#include "stdint.h"
#include "acc_lis3dh.h"
#include "driver/gpio.h"
#include "sensors.h"

typedef esp_err_t (*acc_init_t)(void);
typedef esp_err_t (*acc_read_t)(float *axis_x, float *axis_y, float *axis_z);
typedef esp_err_t (*acc_autoteste_t)(void);
typedef int (*acc_irq_handler_t)(uint8_t io, uint8_t value);
typedef int (*acc_register_irq_t)(void);

struct acc_driver_data_t
{
    struct lis3dh_driver_t *lis3dh_driver;
    QueueHandle_t acc_irq_evt_queue;
    acc_irq_handler_t acc_irq_handler;
    uint8_t count_int_first;
    TaskHandle_t xHandle_acc;
};

struct acc_driver_t
{
    acc_init_t acc_init;
    acc_read_t acc_read;
    acc_register_irq_t acc_register_irq;
    acc_autoteste_t acc_autoteste;
};

struct acc_driver_t *acc_driver_instance();

#endif