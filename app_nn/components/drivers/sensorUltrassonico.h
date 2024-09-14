#ifndef ULTRASONIC_SENSOR_H_
#define ULTRASONIC_SENSOR_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/timer.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_err.h"



#include <stdio.h>

#define TIMER_DIVIDER (2)
#define TIMER_SCALE 8000

#define PWM_OUTPUT_PIN GPIO_NUM_23
#define RECV_ECHO_PIN GPIO_NUM_12
// #define THRESHOLD_PIN GPIO_NUM_21
#define ULTRASONIC_TIMEOUT 1    // 10ms na contagem de ticks do esp32

#define ULTRASONIC_TIMER_GROUP TIMER_GROUP_1
#define ULTRASONIC_TIMER TIMER_1
#define ULTRASONIC_PWM_TIMER LEDC_TIMER_1
#define ULTRASONIC_PWM_CHANNEL LEDC_CHANNEL_1
#define ULTRASONIC_PWM_FREQUENCY 40000
#define CM_CONVERTION_FACTOR 28

#define ULTRASONIC_GPIO_PIN_LOW 0
#define ULTRASONIC_GPIO_PIN_HIGH 1

typedef enum
{
    ULTRASONIC_SUCCESS,
    ULTRASONIC_FAIL
} ultrasonic_status_t;

struct ultrasonic_driver_data_t
{
    timer_group_t  u_timer_group;
    timer_idx_t  u_timer;
    ledc_timer_t u_ledc_timer;
    ledc_channel_t u_ledc_channel;
    uint32_t u_frequency;
    uint8_t u_pwm_pin;
    uint8_t u_echo_pin;
    uint8_t u_threshold_pin;
    uint8_t u_distance_threshold;
};

typedef ultrasonic_status_t (*ultrasonic_init_t)(struct ultrasonic_driver_data_t *driver_data);
typedef uint32_t (*ultrasonic_get_distance_t)(struct ultrasonic_driver_data_t *driver_data);

struct ultrasonic_driver_t
{
    ultrasonic_init_t init;
    ultrasonic_get_distance_t get_distance;
};

struct ultrasonic_driver_t *ultrasonic_driver_instance(void);
struct ultrasonic_driver_data_t ultrasonic_get_attributes(void);

#endif