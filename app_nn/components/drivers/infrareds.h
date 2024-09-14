#ifndef INFRAREDS_DRIVERS_H_
#define INFRAREDS_DRIVERS_H_

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/timer.h"
#include "sensors.h"
//#include "mcp.h"

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define GPIOB                   0x13
    
#define LED_EM_1               (1<<1) 
#define LED_EM_2               (1<<2) 
#define LED_EM_3               (1<<3) 
#define LED_EM_4               (1<<4) 
#define LED_EM_5               (1<<5) 
#define LED_EM_6               (1<<6) 


typedef esp_err_t (*infrareds_init_t)(void);
typedef esp_err_t (*infrareds_autoteste_t)(uint8_t *res_autoteste);

struct infrareds_driver_data_t
{
    uint8_t infrared_data[50];
    TaskHandle_t xHandle_infrared;
};

struct infrareds_driver_t
{
    infrareds_init_t infrareds_init;
    infrareds_autoteste_t infrareds_autoteste;
};

struct infrareds_driver_t *infrareds_driver_instance();

#endif