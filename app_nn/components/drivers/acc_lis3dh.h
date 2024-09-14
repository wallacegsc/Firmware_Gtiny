#ifndef LIS3DH_DRIVER_H_
#define LIS3DH_DRIVER_H_

#include "stdint.h"
#include "stdbool.h"
#include "i2c.h"
#include "esp_log.h"

//Address lis3dh
#define LIS3DH_I2C_ADDRESS_1           0x18  // SDO pin is low
#define LIS3DH_I2C_ADDRESS_2           0x19  // SDO pin is high

//Qual porta I2C o acell irá utilizar
#define LIS3DH_I2C_MACHINE I2C_NUM_0

// LIS3DH chip id
#define LIS3DH_CHIP_ID                 0x33  // LIS3DH_REG_WHO_AM_I<7:0>


// register addresses
#define LIS3DH_REG_STATUS_AUX    0x07
#define LIS3DH_REG_OUT_ADC1_L    0x08
#define LIS3DH_REG_OUT_ADC1_H    0x09
#define LIS3DH_REG_OUT_ADC2_L    0x0a
#define LIS3DH_REG_OUT_ADC2_H    0x0b
#define LIS3DH_REG_OUT_ADC3_L    0x0c
#define LIS3DH_REG_OUT_ADC3_H    0x0d
#define LIS3DH_REG_INT_COUNTER   0x0e
#define LIS3DH_REG_WHO_AM_I      0x0f
#define LIS3DH_REG_TEMP_CFG      0x1f
#define LIS3DH_REG_CTRL1         0x20
#define LIS3DH_REG_CTRL2         0x21
#define LIS3DH_REG_CTRL3         0x22
#define LIS3DH_REG_CTRL4         0x23
#define LIS3DH_REG_CTRL5         0x24
#define LIS3DH_REG_CTRL6         0x25
#define LIS3DH_REG_REFERENCE     0x26
#define LIS3DH_REG_STATUS        0x27
#define LIS3DH_REG_OUT_X_L       0x28
#define LIS3DH_REG_OUT_X_H       0x29
#define LIS3DH_REG_OUT_Y_L       0x2a
#define LIS3DH_REG_OUT_Y_H       0x2b
#define LIS3DH_REG_OUT_Z_L       0x2c
#define LIS3DH_REG_OUT_Z_H       0x2d
#define LIS3DH_REG_FIFO_CTRL     0x2e
#define LIS3DH_REG_FIFO_SRC      0x2f
#define LIS3DH_REG_INT1_CFG      0x30
#define LIS3DH_REG_INT1_SRC      0x31
#define LIS3DH_REG_INT1_THS      0x32
#define LIS3DH_REG_INT1_DUR      0x33
#define LIS3DH_REG_INT2_CFG      0x34
#define LIS3DH_REG_INT2_SRC      0x35
#define LIS3DH_REG_INT2_THS      0x36
#define LIS3DH_REG_INT2_DUR      0x37
#define LIS3DH_REG_CLICK_CFG     0x38
#define LIS3DH_REG_CLICK_SRC     0x39
#define LIS3DH_REG_CLICK_THS     0x3a
#define LIS3DH_REG_TIME_LIMIT    0x3b
#define LIS3DH_REG_TIME_LATENCY  0x3c
#define LIS3DH_REG_TIME_WINDOW   0x3d

/**
 * @brief Resgistrador 0x27 onde os bits de status se encontram
 * 
 */
struct lis3dh_reg_status 
{
    uint8_t XDA   :1;      // STATUS<0>   X axis new data available
    uint8_t YDA   :1;      // STATUS<1>   Y axis new data available
    uint8_t ZDA   :1;      // STATUS<2>   Z axis new data available
    uint8_t ZYXDA :1;      // STATUS<3>   X, Y and Z axis new data available
    uint8_t XOR   :1;      // STATUS<4>   X axis data overrun
    uint8_t YOR   :1;      // STATUS<5>   Y axis data overrun 
    uint8_t ZOR   :1;      // STATUS<6>   Z axis data overrun
    uint8_t ZYXOR :1;      // STATUS<7>   X, Y and Z axis data overrun
};

/**
 * @brief Registrador 0x20 onde se encontram as configuracoes iniciais
 * 
 */
struct lis3dh_reg_ctrl1 
{
    uint8_t Xen  :1;       // CTRL1<0>    X axis enable
    uint8_t Yen  :1;       // CTRL1<1>    Y axis enable
    uint8_t Zen  :1;       // CTRL1<2>    Z axis enable
    uint8_t LPen :1;       // CTRL1<3>    Low power mode enable
    uint8_t ODR  :4;       // CTRL1<7:4>  Data rate selection
};

/**
 * @brief Registrador 0x21
 * 
 */
struct lis3dh_reg_ctrl2 
{
    uint8_t HPIS1   :1;    // CTRL2<0>    HPF enabled for AOI on INT2
    uint8_t HPIS2   :1;    // CTRL2<1>    HPF enabled for AOI on INT2
    uint8_t HPCLICK :1;    // CTRL2<2>    HPF enabled for CLICK
    uint8_t FDS     :1;    // CTRL2<3>    Filter data selection
    uint8_t HPCF    :2;    // CTRL2<5:4>  HPF cutoff frequency
    uint8_t HPM     :2;    // CTRL2<7:6>  HPF mode
};

/**
 * @brief Registrador 0x22
 * 
 */
struct lis3dh_reg_ctrl3 
{
    uint8_t unused     :1; // CTRL3<0>  unused
    uint8_t I1_OVERRUN :1; // CTRL3<1>  FIFO Overrun interrupt on INT1
    uint8_t I1_WTM1    :1; // CTRL3<2>  FIFO Watermark interrupt on INT1
    uint8_t IT_DRDY2   :1; // CTRL3<3>  DRDY2 (ZYXDA) interrupt on INT1
    uint8_t IT_DRDY1   :1; // CTRL3<4>  DRDY1 (321DA) interrupt on INT1
    uint8_t I1_AOI2    :1; // CTRL3<5>  AOI2 interrupt on INT1
    uint8_t I1_AOI1    :1; // CTRL3<6>  AOI1 interrupt on INT1
    uint8_t I1_CLICK   :1; // CTRL3<7>  CLICK interrupt on INT1
};

/**
 * @brief Registrador 0x23
 * 
 */
struct lis3dh_reg_ctrl4 
{
    uint8_t SIM :1;        // CTRL4<0>   SPI serial interface selection
    uint8_t ST  :2;        // CTRL4<2:1> Self test enable
    uint8_t HR  :1;        // CTRL4<3>   High resolution output mode
    uint8_t FS  :2;        // CTRL4<5:4> Full scale selection
    uint8_t BLE :1;        // CTRL4<6>   Big/litle endian data selection
    uint8_t BDU :1;        // CTRL4<7>   Block data update
};

/**
 * @brief Registrador 0x24
 * 
 */
struct lis3dh_reg_ctrl5 
{
    uint8_t D4D_INT2 :1;   // CTRL5<0>   4D detection enabled on INT1
    uint8_t LIR_INT2 :1;   // CTRL5<1>   Latch interrupt request on INT1
    uint8_t D4D_INT1 :1;   // CTRL5<2>   4D detection enabled on INT2
    uint8_t LIR_INT1 :1;   // CTRL5<3>   Latch interrupt request on INT1
    uint8_t unused   :2;   // CTRL5<5:4> unused
    uint8_t FIFO_EN  :1;   // CTRL5<6>   FIFO enabled
    uint8_t BOOT     :1;   // CTRL5<7>   Reboot memory content
};

/**
 * @brief Registrador 0x25
 * 
 */
struct lis3dh_reg_ctrl6 
{
    uint8_t unused1  :1;   // CTRL6<0>   unused
    uint8_t H_LACTIVE:1;   // CTRL6<1>   Interrupt polarity
    uint8_t unused2  :1;   // CTRL6<2>   unused
    uint8_t I2_ACT   :1;   // CTRL6<3>   ?
    uint8_t I2_BOOT  :1;   // CTRL6<4>   ?
    uint8_t I2_AOI2  :1;   // CTRL6<5>   AOI2 interrupt on INT1
    uint8_t I2_AOI1  :1;   // CTRL6<6>   AOI1 interrupt on INT1
    uint8_t I2_CLICK :1;   // CTRL6<7>   CLICK interrupt on INT2
};

/**
 * @brief Registrador 0x2E
 * 
 */
struct lis3dh_reg_fifo_ctrl
{
    uint8_t FTH :5;        // FIFO_CTRL<4:0>  FIFO threshold
    uint8_t TR  :1;        // FIFO_CTRL<5>    Trigger selection INT1 / INT2
    uint8_t FM  :2;        // FIFO_CTRL<7:6>  FIFO mode
};

/**
 * @brief Registrador 0x2F
 * 
 */
struct lis3dh_reg_fifo_src
{
    uint8_t FFS       :5;  // FIFO_SRC<4:0>  FIFO samples stored
    uint8_t EMPTY     :1;  // FIFO_SRC<5>    FIFO is empty
    uint8_t OVRN_FIFO :1;  // FIFO_SRC<6>    FIFO buffer full
    uint8_t WTM       :1;  // FIFO_SRC<7>    FIFO content exceeds watermark
};

/**
 * @brief Registrador 0x34
 * 
 */
struct lis3dh_reg_intx_cfg
{
    uint8_t XLIE :1;   // INTx_CFG<0>    X axis below threshold enabled
    uint8_t XHIE :1;   // INTx_CFG<1>    X axis above threshold enabled
    uint8_t YLIE :1;   // INTx_CFG<2>    Y axis below threshold enabled
    uint8_t YHIE :1;   // INTx_CFG<3>    Y axis above threshold enabled
    uint8_t ZLIE :1;   // INTx_CFG<4>    Z axis below threshold enabled
    uint8_t ZHIE :1;   // INTx_CFG<5>    Z axis above threshold enabled
    uint8_t SIXD :1;   // INTx_CFG<6>    6D/4D orientation detecetion enabled
    uint8_t AOI  :1;   // INTx_CFG<7>    AND/OR combination of interrupt events
};

/**
 * @brief Registrador 0x35
 * 
 */
struct lis3dh_reg_intx_src
{
    uint8_t XL    :1;  // INTx_SRC<0>    X axis below threshold enabled
    uint8_t XH    :1;  // INTx_SRC<1>    X axis above threshold enabled
    uint8_t YL    :1;  // INTx_SRC<2>    Y axis below threshold enabled
    uint8_t YH    :1;  // INTx_SRC<3>    Y axis above threshold enabled
    uint8_t ZL    :1;  // INTx_SRC<4>    Z axis below threshold enabled
    uint8_t ZH    :1;  // INTx_SRC<5>    Z axis above threshold enabled
    uint8_t IA    :1;  // INTx_SRC<6>    Interrupt active
    uint8_t unused:1;  // INTx_SRC<7>    unused
};

/**
 * @brief Registrador 0x38
 * 
 */
struct lis3dh_reg_click_cfg
{
    uint8_t XS    :1;  // CLICK_CFG<0>    X axis single click enabled
    uint8_t XD    :1;  // CLICK_CFG<1>    X axis double click enabled
    uint8_t YS    :1;  // CLICK_CFG<2>    Y axis single click enabled
    uint8_t YD    :1;  // CLICK_CFG<3>    Y axis double click enabled
    uint8_t ZS    :1;  // CLICK_CFG<4>    Z axis single click enabled
    uint8_t ZD    :1;  // CLICK_CFG<5>    Z axis double click enabled
    uint8_t unused:2;  // CLICK_CFG<7:6>  unused
};

struct lis3dh_config_data_t
{
    struct lis3dh_reg_status    struct_status_axis;
    struct lis3dh_reg_ctrl1     struct_enable_axis;
    struct lis3dh_reg_ctrl3     struct_enable_int;
    struct lis3dh_reg_ctrl5     struct_other_config;
    struct lis3dh_reg_fifo_ctrl struct_fifo_ctrl;
    struct lis3dh_reg_fifo_src  struct_fifo_src;
};

/**
 * @brief Prototipo de estrutura de dados do lis3dh
 * 
 */
struct lis3dh_driver_data_t
{
    struct i2c_controller_t *lis3dh_i2c;
    struct lis3dh_config_data_t lis3dh_config_data;
};

esp_err_t lis3dh_config(void);
typedef esp_err_t (*lis3dh_init_t)(void);
typedef esp_err_t (*lis3dh_read_data_t)(int16_t *data_acc);
typedef esp_err_t (*lis3dh_status_data_t)(uint8_t *ready);
typedef esp_err_t (*lis3dh_status_fifo_t)(uint8_t *fifoData);

/**
 * @brief Prototipo de estrutura de métodos do lis3dh
 * 
 */
struct lis3dh_driver_t
{
    lis3dh_init_t lis3dh_init;
    lis3dh_status_data_t lis3dh_status_data;
    lis3dh_read_data_t lis3dh_read_data;
    lis3dh_status_fifo_t lis3dh_status_fifo;
};

struct lis3dh_driver_t *lis3dh_driver_instance();


#endif