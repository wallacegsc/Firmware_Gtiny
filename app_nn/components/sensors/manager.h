#ifndef MANAGER_H_
#define MANAGER_H_

#include "sensors.h"

#define GUARDIAO_TINY_4_0

#ifdef GUARDIAO_TINY_4_0
    #define HW_VERSION "GUARDIAO_ESP_V4.0"
    #define FW_VERSION "1.0"
#endif

struct manager_data_t
{
    
};

typedef esp_err_t (*manager_init_t)();

struct manager_t
{
    manager_init_t manager_init;
};

struct manager_t *manager_instance();

#endif