#ifndef EEPROM_H_
#define EEPROM_H_

#include "i2c.h"
#include <stdint.h>

#define I2C_MASTER_ACK_ENABLE   1
#define EEPROM_WRITE_ADDR   0
#define EEPROM_READ_ADDR    1
#define EEPROM_PAGE_SIZE	256 //bytes
#define EEPROM_FULL_SIZE    131072 //131072


struct eeprom_driver_data_t
{
    // i2c_dev_t i2c_dev;
    //  maxSize eeprom size
    //  etc, etc, etc
    //  dados relacionados especificamente ao driver da eeprom
};

struct eeprom_driver_t *eeprom_driver_instance();

typedef int (*eeprom_init_t)();
typedef int (*read_byte_t)(uint32_t addr, uint8_t *data);
typedef int (*write_byte_t)(uint32_t addr, uint8_t data);
typedef int (*read_buffer_t)(uint32_t addr, uint8_t *data, uint32_t size);
typedef int (*write_buffer_t)(uint32_t addr, uint8_t *data, uint32_t size);

struct eeprom_driver_t
{
    eeprom_init_t init;
    read_byte_t read_byte;
    write_byte_t write_byte;
    read_buffer_t read_buffer;
    write_buffer_t write_buffer;
};

esp_err_t eeprom_write_byte(uint8_t deviceaddress, uint16_t eeaddress, uint8_t byte);
esp_err_t eeprom_write(uint8_t deviceaddress, uint32_t eeaddress, uint8_t* data, size_t size);

esp_err_t eeprom_read_byte(uint8_t deviceaddress, uint16_t eeaddress,uint8_t *byte);
esp_err_t eeprom_read(uint8_t deviceaddress, uint32_t eeaddress, uint8_t* data, size_t size);


#endif