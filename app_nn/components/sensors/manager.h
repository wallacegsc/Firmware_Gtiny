#ifndef MANAGER_H_
#define MANAGER_H_

#include "sensors.h"

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