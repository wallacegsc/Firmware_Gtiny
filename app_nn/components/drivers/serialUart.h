#ifndef SERIALUART_DRIVER_H_
#define SERIALUART_DRIVER_H_

#include "stdint.h"
#include "esp_log.h"
#include "string.h"

#include "driver/uart.h"
#include "sensors.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "mbedtls/aes.h"

struct serialUart_driver_data_t
{
    uint8_t res_autoteste[8];
    uint8_t predict_model, predict_model_history;
};

typedef esp_err_t (*serial_uart_init_t)(void);
typedef esp_err_t (*serial_uart_write_t)(char *buffer_tx, uint32_t size);

struct serialUart_driver_t
{
    serial_uart_init_t serial_uart_init;
    serial_uart_write_t serial_uart_write;
    
};

struct serialUart_driver_t *serialUart_driver_instance();
struct serialUart_driver_data_t serialUart_data_driver;

#endif