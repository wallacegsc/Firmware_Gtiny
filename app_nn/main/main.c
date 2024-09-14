#include "manager.h"
#include "serialUart.h"

void app_main(void)
{
    struct manager_t *manager_guardian = manager_instance();
    manager_guardian->manager_init();

    // struct serialUart_driver_t *serialUart = serialUart_driver_instance();


}