#include "acc_lis3dh.h"


/**
 * @brief Criação dos objetos LIS3DH
 * 
 */
struct lis3dh_driver_data_t lis3dh_driver_data;
struct lis3dh_driver_t lis3dh_driver;

/**
 * @brief isso deve ser substituido por macros
 * ******************************************************
 * ****************************************************** 
 */

struct i2c_controller_t *lis3dh_get_i2c_controller()
{
    return lis3dh_driver_data.lis3dh_i2c;
}

struct lis3dh_config_data_t *lis3dh_get_struct_config_data()
{
    return &lis3dh_driver_data.lis3dh_config_data;
}


/**
 * ******************************************************
 * ******************************************************
 */

/**
 * @brief inicialização do acelerometro
 * 
 * @return int 
 */
esp_err_t lis3dh_init(void)
{   
    lis3dh_driver_data.lis3dh_i2c = i2c_controller_instance();

    struct i2c_controller_t *i2c_controller = lis3dh_get_i2c_controller();

    uint8_t lis3dh_id;

    if(i2c_controller->i2c_read_8b(LIS3DH_I2C_MACHINE, LIS3DH_I2C_ADDRESS_2, LIS3DH_REG_WHO_AM_I, &lis3dh_id) != ESP_OK)
    {
        ESP_LOGE("LIS3DH_init", "Falha na comunicação");
        return ESP_FAIL;
    }

    if(lis3dh_id != LIS3DH_CHIP_ID)
    {
        ESP_LOGE("LIS3DH_init", "Dispositivo não encontrado");
        return ESP_FAIL;
    }

    ESP_LOGI("LIS3DH_init", "Dispositivo encontrado");
    lis3dh_config();
    
    return ESP_OK;
    
}

/**
 * @brief configuração inicial do acelerometro
 * 
 */
esp_err_t lis3dh_config(void)
{

    struct lis3dh_config_data_t *lis3dh_struct_config = lis3dh_get_struct_config_data();
    struct i2c_controller_t *i2c_controller = lis3dh_get_i2c_controller();
    uint8_t buffer;


    /**************************************CONFIG************************************************/
    //Habilitando os eixos do acelerometro
    lis3dh_struct_config->struct_enable_axis.Xen = 1;
    lis3dh_struct_config->struct_enable_axis.Yen = 1;
    lis3dh_struct_config->struct_enable_axis.Zen = 1;

    //Data rate configuration 50Hz
    lis3dh_struct_config->struct_enable_axis.ODR = 4;
    
    //buffer para habilitar os eixos
    buffer = lis3dh_struct_config->struct_enable_axis.ODR << 4 | lis3dh_struct_config->struct_enable_axis.Zen << 2 | lis3dh_struct_config->struct_enable_axis.Yen << 1 | lis3dh_struct_config->struct_enable_axis.Xen << 0; 

    //Escrevendo o buffer de configurações dos eixos
    esp_err_t err_config = i2c_controller->i2c_write(LIS3DH_I2C_MACHINE, LIS3DH_I2C_ADDRESS_2, LIS3DH_REG_CTRL1, buffer);

    if (err_config != ESP_OK)
    {
        ESP_LOGE("LIS3DH_config", "Erro de configuração");
        return ESP_FAIL;
    }

    ESP_LOGI("LIS3DH_config", "LIS3DH configurado com sucesso");
    
    /****************************************************INT FIFO**********************************************************/
    
    //Habilitar interrupções para modo FIFO
    lis3dh_struct_config->struct_enable_int.I1_OVERRUN = 0;
    lis3dh_struct_config->struct_enable_int.I1_WTM1 = 1;

    //buffer para habilitar as interrupções FIFO
    buffer = 0;
    buffer = lis3dh_struct_config->struct_enable_int.I1_OVERRUN << 1 | lis3dh_struct_config->struct_enable_int.I1_WTM1 << 2; 

    err_config = i2c_controller->i2c_write(LIS3DH_I2C_MACHINE, LIS3DH_I2C_ADDRESS_2, LIS3DH_REG_CTRL3, buffer);

    if (err_config != ESP_OK)
    {
        ESP_LOGE("LIS3DH_config", "Erro de configuração das interrupções");
        return ESP_FAIL;
    }

    ESP_LOGI("LIS3DH_config", "Interrupções do LIS3DH configuradas com sucesso");

    /**************************************************FIFO******************************************************************/
    //Habilitar modo FIFO
    lis3dh_struct_config->struct_other_config.FIFO_EN = 1;
    //buffer para habilitar a FIFO
    buffer = 0;
    buffer = lis3dh_struct_config->struct_other_config.FIFO_EN << 6; 

    err_config = i2c_controller->i2c_write(LIS3DH_I2C_MACHINE, LIS3DH_I2C_ADDRESS_2, LIS3DH_REG_CTRL5, buffer);

    if (err_config != ESP_OK)
    {
        ESP_LOGE("LIS3DH_config", "Erro de habilitação da FIFO");
        return ESP_FAIL;
    }

    ESP_LOGI("LIS3DH_config", "FIFO habilitada com sucesso");

    /**********************************************FIFO CONFIG*****************************************************************/

    lis3dh_struct_config->struct_fifo_ctrl.FTH = 24;
    lis3dh_struct_config->struct_fifo_ctrl.TR = 0;
    lis3dh_struct_config->struct_fifo_ctrl.FM = 0x02;

    //buffer para configurar a FIFO
    buffer = 0;
    buffer = lis3dh_struct_config->struct_fifo_ctrl.FTH << 0 | lis3dh_struct_config->struct_fifo_ctrl.TR << 5 | lis3dh_struct_config->struct_fifo_ctrl.FM << 6; 

    err_config = i2c_controller->i2c_write(LIS3DH_I2C_MACHINE, LIS3DH_I2C_ADDRESS_2, LIS3DH_REG_FIFO_CTRL, buffer);

    if (err_config != ESP_OK)
    {
        ESP_LOGE("LIS3DH_config", "Erro de configuração da FIFO");
        return ESP_FAIL;
    }

    ESP_LOGI("LIS3DH_config", "FIFO configurada com sucesso");
    return ESP_OK;      
}

/**
 * @brief Retorna se os dados dos três eixos esstão disponíveis
 * 
 * @param ready 
 * @return esp_err_t 
 */
esp_err_t lis3dh_status_data(uint8_t *ready)
{
    struct i2c_controller_t *i2c_controller = lis3dh_get_i2c_controller();

    if (i2c_controller->i2c_read_8b(LIS3DH_I2C_MACHINE, LIS3DH_I2C_ADDRESS_2, LIS3DH_REG_STATUS, ready) != ESP_OK)
    {
        ESP_LOGE("LIS3DH_status", "Falha na leitura do status dos dados");
        return ESP_FAIL;
    }

    //ESP_LOGI("LIS3DH_status", "retornando o status dos sensores");
    return ESP_OK;    
}

/**
 * @brief 
 * 
 * @param lis3dh 
 * @param address_msb 
 * @param address_lsb 
 * @return uint16_t 
 */
esp_err_t lis3dh_read_data(int16_t *data_acc)
{
    struct i2c_controller_t *i2c_controller = lis3dh_get_i2c_controller();

    if(i2c_controller->i2c_read_16b(LIS3DH_I2C_MACHINE, LIS3DH_I2C_ADDRESS_2, LIS3DH_REG_OUT_X_H, LIS3DH_REG_OUT_X_L, &data_acc[0]) != ESP_OK)
    {
        ESP_LOGE("LIS3DH_read", "Falha na leitura do eixo x");
        return ESP_FAIL;
    }
    data_acc[0] = data_acc[0] >> 4;

    if(i2c_controller->i2c_read_16b(LIS3DH_I2C_MACHINE, LIS3DH_I2C_ADDRESS_2, LIS3DH_REG_OUT_Y_H, LIS3DH_REG_OUT_Y_L, &data_acc[1]) != ESP_OK)
    {
        ESP_LOGE("LIS3DH_read", "Falha na leitura do eixo y");
        return ESP_FAIL;
    }
    data_acc[1] = data_acc[1] >> 4;

    if(i2c_controller->i2c_read_16b(LIS3DH_I2C_MACHINE, LIS3DH_I2C_ADDRESS_2, LIS3DH_REG_OUT_Z_H, LIS3DH_REG_OUT_Z_L, &data_acc[2]) != ESP_OK)
    {
        ESP_LOGE("LIS3DH_read", "Falha na leitura do eixo x");
        return ESP_FAIL;
    }
    data_acc[2] = data_acc[2] >> 4;


    return ESP_OK;
}

/*
    struct i2c_controller_t *i2c_controller = lis3dh_get_i2c_controller();
    uint8_t buff[6];

    if(i2c_controller->i2c_read_bytes(LIS3DH_I2C_MACHINE, LIS3DH_I2C_ADDRESS_2, LIS3DH_REG_OUT_X_L, buff, 6) != ESP_OK)
    {
        ESP_LOGE("LIS3DH_read", "Falha na leitura do eixo x");
        return ESP_FAIL;
    }

    data_acc[0] = (int16_t)((buff[1] << 8) | (buff[0] & 0xC0));
    data_acc[1] = (int16_t)((buff[3] << 8) | (buff[2] & 0xC0));
    data_acc[2] = (int16_t)((buff[5] << 8) | (buff[4] & 0xC0));
*/

/**
 * @brief 
 * 
 * @param fifoData 
 * @return esp_err_t 
 */
esp_err_t lis3dh_status_fifo(uint8_t *fifoData)
{
    struct i2c_controller_t *i2c_controller = lis3dh_get_i2c_controller();
    if(i2c_controller->i2c_read_8b(LIS3DH_I2C_MACHINE, LIS3DH_I2C_ADDRESS_2, LIS3DH_REG_FIFO_SRC, fifoData) != ESP_OK)
    {
        ESP_LOGE("LIS3DH_read", "Falha na leitura do eixo x");
        return ESP_FAIL;
    }

    //ESP_LOGI("LIS3DH_FIFO", "Leitura do registro de configurações do fifo");
    return ESP_OK;
    
}



struct lis3dh_driver_t *lis3dh_driver_instance()
{
    if(lis3dh_driver.lis3dh_init == 0)
    {
        lis3dh_driver = (struct lis3dh_driver_t){
            .lis3dh_init = &lis3dh_init,
            .lis3dh_status_data = &lis3dh_status_data,
            .lis3dh_read_data = &lis3dh_read_data,
            .lis3dh_status_fifo = &lis3dh_status_fifo,
        };
    }
    return &lis3dh_driver;
}