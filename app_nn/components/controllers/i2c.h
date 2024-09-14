#ifndef I2C_CONTROLLER_H_
#define I2C_CONTROLLER_H_

#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_MASTER_SDA_IO_ACCEL       19
#define I2C_MASTER_SCL_IO_ACCEL       18

#define I2C_MASTER_SDA_IO_MCP         22
#define I2C_MASTER_SCL_IO_MCP         21

#define I2C_MASTER_READ_BIT     1
#define I2C_MASTER_WRITE_BIT    0

#define I2C_MASTER_ACK_ENABLE   1
#define I2C_MASTER_ACK_DISABLE  0

#define I2C_AUTO_INCREMENT (0x80)

/**
 * @brief Prototipo de estrutura dos atributos
 * 
 */
struct i2c_controller_data_t
{
    SemaphoreHandle_t xSemaphore_i2c;
};

/**
 * @brief Prototipos de funções
 * 
 */
typedef esp_err_t (*i2c_init_t)(void);
typedef esp_err_t (*i2c_read_8b_t)(i2c_port_t machine_i2c, uint8_t address, uint8_t addr_reg_data, uint8_t *value_data);
typedef esp_err_t (*i2c_read_16b_t)(i2c_port_t machine_i2c, uint8_t address, uint8_t addr_reg_data_msb, uint8_t addr_reg_data_lsb, int16_t *value_data);
typedef esp_err_t (*i2c_read_bytes_t)(i2c_port_t machine_i2c, uint8_t address, uint8_t addr_reg_data, uint8_t *value_data, uint8_t len);
typedef esp_err_t (*i2c_write_t)(i2c_port_t machine_i2c, uint8_t address, uint8_t addr_reg_data, uint8_t word);

/**
 * @brief Prototipo de estrutura dos metodos
 * 
 */
struct i2c_controller_t
{
    i2c_init_t i2c_init;
    i2c_read_8b_t i2c_read_8b;
    i2c_read_8b_t i2c_read_8b_mcp;
    i2c_read_16b_t i2c_read_16b;
    i2c_read_bytes_t i2c_read_bytes;
    i2c_write_t i2c_write;
    i2c_write_t i2c_write_mcp;
};

struct i2c_controller_t *i2c_controller_instance();

#endif