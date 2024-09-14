#include "manager.h"

struct manager_data_t manager_data;
struct manager_t manager;

esp_err_t manager_init(void)
{
    struct sensors_t *sensors_guardian = sensors_instance();
    
    sensors_guardian->sensors_init();

    while(1)
    {
        vTaskDelay(5000/portTICK_PERIOD_MS);
        break;
    }

    sensors_guardian->sensors_autoteste();
    return ESP_OK;
}

struct manager_t *manager_instance()
{
    if(manager.manager_init == 0)
    {
        manager = (struct manager_t)
        {
            .manager_init = &manager_init,
        };
    }
    return &manager;
}