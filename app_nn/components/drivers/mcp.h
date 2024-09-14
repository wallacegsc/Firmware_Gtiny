
/*
 * mcp_23018.c
 *
 *  Created on: May 20, 2022
 *      Author: Maysa Jhiovanna
 */

#include "i2c.h"
#include "stdint.h"
#include "eeprom.h"


#define GPIOA                   0x12                                                               // GENERAL PURPOSE I/O PORT-A Register
#define GPIOB                   0x13                                                               // GENERAL PURPOSE I/O PORT-B Register
#define IODIRA                  0x00                                                               // I/O DIRECTION-A Register                                
#define IODIRB                  0x01                                                               // I/O DIRECTION-A Register                                
#define ALL_RESET               0xff      
#define ALL_SET                 0x00      
#define IOCONA                  0x0a                                                               // I/O EXPANDER CONFIGURATION-A Register
#define GPPUA                   0x0c
#define GPPUB                   0x0d 

#define IPOLA                   0x02                                                             // INPUT POLARITY PORT-A Register
#define GPINTENA                0x04                                                             // INTERRUPT-ON-CHANGE-A PINS
#define DEFVALA                 0x06                                                             // DEFAULT VALUE-A Register
#define INTCONA                 0x08                                                             // INTERRUPT-ON-CHANGE CONTROL-A Register
#define INTFA                   0x0E                                                             // INTERRUPT FLAG-A Register
#define INTCAPA                 0x10                                                             // INTERRUPT CAPTURED VALUE FOR PORT-A Register

#define IPOLB                   0x03                                                             // INPUT POLARITY PORT-B Register
#define GPINTENB                0x05                                                             // INTERRUPT-ON-CHANGE-B PINS 
#define DEFVALB                 0x07                                                             // DEFAULT VALUE-B Register
#define INTCONB                 0x09                                                             // INTERRUPT-ON-CHANGE CONTROL-B Register
#define INTFB                   0x0F                                                             // INTERRUPT FLAG-B Register
#define OLATB                   0x15                                                             // OUTPUT LATCH-B Register 0

#define P0                     (1)    
#define P1                     (1<<1) 
#define P2                     (1<<2) 
#define P3                     (1<<3) 
#define P4                     (1<<4) 
#define P5                     (1<<5) 
#define P6                     (1<<6) 
#define P7                     (1<<7)

#define SDA_PIN                  22                                                                    /*Barramento de dados*/
#define SCL_PIN                  21                                                                    /*Barramento do clock*/
#define MCP_I2C_ADDRESS          0x20                                                                    /*Register para ler os pinos do escravo*/ 
#define READ_BIT                 0x1                                                                   /*Para ler o pino do escravo deve-se definir em 1 o bit de R\W do primeiro byte do comando*/
#define WRITE_BIT                0x0                                                                   /*Para escrever no escravo deve-se definir em 0 o bit de R\W do primeiro byte do comando*/
#define ACK_CHECK_EN             0x1                                                                   /*I2C master irá verificar o acknowledge do escravo*/
#define OUTPUT                   0
#define INPUT                    1
#define HIGH                     1
#define LOW                      0
#define ENABLE                   1
#define DISABLE                  0
#define INTA                     21
#define RISING                   1
#define FALLING                  0
#define CHANGE                   1

#define MCP_I2C_MACHINE      I2C_NUM_1
#define PLACA_CONCENTRADORA 1

typedef esp_err_t (*mcp_driver_init_t)(void);
typedef esp_err_t (*mode_mcp_t)(uint8_t port, uint8_t pin, uint8_t value);
typedef esp_err_t (*write_pin_mcp_t)(uint8_t port, uint8_t pin, uint8_t value);
typedef esp_err_t (*write_group_mcp_t)(uint8_t port, uint8_t value);
typedef esp_err_t (*read_pin_mcp_t)(uint8_t port, uint8_t pin, uint8_t *read);
typedef esp_err_t (*read_group_mcp_t)(uint8_t port, uint8_t *read);
typedef esp_err_t (*pullup_pin_t)(uint8_t port, uint8_t pin, bool value);


struct mcp_driver_t
{
    mcp_driver_init_t mcp_driver_init;
    mode_mcp_t mode_mcp;
    write_pin_mcp_t write_pin_mcp;
    write_group_mcp_t write_group_mcp;
    read_pin_mcp_t read_pin_mcp;
    read_group_mcp_t read_group_mcp;
    pullup_pin_t pullup_pin;
};

struct mcp_driver_t *mcp_driver_instance();