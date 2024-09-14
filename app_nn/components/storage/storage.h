#ifndef STORAGE_H_
#define STORAGE_H_
#include "eeprom.h"
#include "time.h"

#define PAGES_EEPROM 4096
#define ADDRESS_EEPROM 0x50
#define EVENT_1 1
#define EVENT_2 2
#define EVENT_3 3
#define EVENT_4 4
#define SAVE_EEPROM 1

#define SIZE_LOG 10
#define ADDRESS_LAST_LOG 131050
#define MAX_LOGS 13105
#define flag_default_relay  160  //10100000 int8
#define flag_inverter_relay 172  //10101100 int8
#define flag_default_event  163  //10100011 int8
#define flag_inverter_event 175  //10101111 int8

// Flag ataque -> xxxxxx00
// Flag event  -> xxxxxx11
// Flag time   -> xxxx00xx
// default
// Flag time   -> xxxx11xx
// inverter 
// Flag identif.> 1010xxxx

struct storage_data_t
{
    QueueHandle_t save_logs_queue;
    uint32_t manager_eeprom;
    uint32_t pos_recent_log; // Log mais recente cadastrado
    uint32_t pos_old_log; // Log mais antigo cadastrado,
    uint16_t memory_full;
    uint8_t  flag_is_inverted;

    SemaphoreHandle_t xSemaphore_eeprom;

};

typedef int (*storage_init_t)(void); //void/*struct eeprom_driver_t *eeprom_driver*/
typedef int (*save_settings_t)(void *settings);
typedef int (*load_settings_t)(void *settings);
typedef int (*save)(void *settings);
typedef esp_err_t (*load_memory_t)(uint16_t num_logs, uint8_t *response);
typedef esp_err_t (*save_data_t)(uint8_t *data, size_t size);
typedef esp_err_t (*load_data_t)(uint8_t *data, size_t size, uint32_t address);
typedef void (*check_num_logs_t)(uint16_t *required_logs);

struct storage_device_t
{
    storage_init_t storage_init;
    save_settings_t save_settings;
    load_settings_t load_settings;
    save_data_t save_data;
    load_data_t load_data;
    load_memory_t load_memory;
    check_num_logs_t check_num_logs;
};

struct storage_device_t *storage_device_instance();

#endif
