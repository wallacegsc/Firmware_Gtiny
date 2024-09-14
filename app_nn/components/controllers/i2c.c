#include "i2c.h"

/**
 * @brief criação de estruturas relacionadas ao i2c
 * 
 */
struct i2c_controller_data_t i2c_controller_data;
struct i2c_controller_t i2c_controller;

/**
 * @brief Inicializacao do barramento I2C
 * 
 * @return esp_err_t 
 */
esp_err_t i2c_init(void)
{
    i2c_config_t i2c_master_config_accel = 
    {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO_ACCEL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO_ACCEL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 1000000,
    };

    i2c_config_t i2c_master_config_mcp = 
    {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO_MCP,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO_MCP,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };

    i2c_param_config(I2C_NUM_0, &i2c_master_config_accel);
    i2c_param_config(I2C_NUM_1, &i2c_master_config_mcp);

    esp_err_t err_accel = i2c_driver_install(I2C_NUM_0, i2c_master_config_accel.mode, 0, 0, 0);
    esp_err_t err_mcp = i2c_driver_install(I2C_NUM_1, i2c_master_config_mcp.mode, 0, 0, 0);
    
    if (err_accel != ESP_OK || err_mcp != ESP_OK)
    {
        ESP_LOGE("I2C", "Barramento i2c com erro");
        return ESP_FAIL;
    }

    i2c_controller_data.xSemaphore_i2c = xSemaphoreCreateMutex();

    ESP_LOGI("I2C", "Barramento i2c inicializado");
    
    return ESP_OK;
}

/**
 * @brief leitura de 1 byte do i2c
 * @param machine_i2c qual maquina i2c sera utilizada
 * @param address endereço i2c do escravo
 * @param addr_reg_data endereço do registrador que será acessado
 * @param value_data ponteiro de resposta da funcao
 * @return esp_err_t
 */
esp_err_t i2c_read_8b(i2c_port_t machine_i2c, uint8_t address, uint8_t addr_reg_data, uint8_t *value_data_8b)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    uint8_t reg_data = 0x00;

    i2c_master_start(cmd);    
    
    esp_err_t err_write = i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE_BIT, I2C_MASTER_ACK_ENABLE);
    
    if (err_write != ESP_OK)
    {
        ESP_LOGE("I2C", "Barramento i2c com erro");
        return ESP_FAIL;
    }

    i2c_master_write_byte(cmd, addr_reg_data, I2C_MASTER_ACK_ENABLE);
    i2c_master_stop(cmd);

    i2c_master_cmd_begin(machine_i2c, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    esp_err_t err_read = i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ_BIT, I2C_MASTER_ACK_ENABLE);
    
    if (err_read != ESP_OK)
    {
        ESP_LOGE("I2C", "Barramento i2c com erro");
        return ESP_FAIL;
    }
    
    i2c_master_read_byte(cmd, &reg_data, I2C_MASTER_ACK_ENABLE);
    i2c_master_stop(cmd);

    i2c_master_cmd_begin(machine_i2c, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    *value_data_8b = reg_data;
    xSemaphoreGive(i2c_controller_data.xSemaphore_i2c);

    
    return ESP_OK;
}

esp_err_t i2c_read_8b_mcp(i2c_port_t machine_i2c, uint8_t address, uint8_t addr_reg_data, uint8_t *value_data_8b)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    uint8_t reg_data = 0x00;

    i2c_master_start(cmd);    
    
    esp_err_t err_write = i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE_BIT, I2C_MASTER_ACK_ENABLE);
    
    if (err_write != ESP_OK)
    {
        ESP_LOGE("I2C", "Barramento i2c com erro");
        return ESP_FAIL;
    }

    i2c_master_write_byte(cmd, addr_reg_data, I2C_MASTER_ACK_ENABLE);
    i2c_master_stop(cmd);

    i2c_master_cmd_begin(machine_i2c, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    esp_err_t err_read = i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ_BIT, I2C_MASTER_ACK_ENABLE);
    
    if (err_read != ESP_OK)
    {
        ESP_LOGE("I2C", "Barramento i2c com erro");
        return ESP_FAIL;
    }
    
    i2c_master_read_byte(cmd, &reg_data, I2C_MASTER_ACK_ENABLE);
    i2c_master_stop(cmd);

    i2c_master_cmd_begin(machine_i2c, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    *value_data_8b = reg_data;
    
    return ESP_OK;
}

/**
 * @brief leitura de 2 bytes do i2c
 * @param machine_i2c qual maquina i2c sera utilizada
 * @param address endereço i2c do escravo
 * @param addr_reg_data_msb endereço msb do registrador que será acessado
 * @param addr_reg_data_lsb endereço lsb do registrador que será acessado
 * @param value_data ponteiro de resposta da funcao
 * @return esp_err_t
 */
esp_err_t i2c_read_16b(i2c_port_t machine_i2c, uint8_t address, uint8_t addr_reg_data_msb, uint8_t addr_reg_data_lsb, uint16_t *value_data)
{
    uint16_t buffer_uint16 = 0x00;
    uint8_t curr_buffer = 0x00;

    /* leitura do byte LSB */
    if(i2c_read_8b(machine_i2c, address, addr_reg_data_lsb, &curr_buffer) != ESP_OK)
    {
        ESP_LOGE("I2C","Erro de leitura");
        return ESP_FAIL;
    }
    
    buffer_uint16 = curr_buffer;
    curr_buffer = 0x00;
    
    /* leitura do byte MSB */
    if(i2c_read_8b(machine_i2c, address, addr_reg_data_msb, &curr_buffer) != ESP_OK)
    {
        ESP_LOGE("I2C","Erro de leitura");
        return ESP_FAIL;
    }
    
    buffer_uint16 |= curr_buffer << 8;
    *value_data = buffer_uint16;
    return ESP_OK;

}

/**
 * @brief leitura de bytes do barramento i2c
 * @param machine_i2c qual maquina i2c sera utilizada
 * @param address endereço i2c do escravo
 * @param addr_reg_data endereço do registrador que será acessado
 * @param bufp ponteiro de resposta da funcao
 * @param len tamanho da reposta
 * @return esp_err_t
 */
esp_err_t i2c_read_bytes(i2c_port_t machine_i2c, uint8_t address, uint8_t addr_reg_data, uint8_t *bufp, uint8_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);    
    
    esp_err_t err_write = i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE_BIT, I2C_MASTER_ACK_ENABLE);
    
    if (err_write != ESP_OK)
    {
        ESP_LOGE("I2C", "Barramento i2c com erro");
        return ESP_FAIL;
    }

    if (len > 1)
    {
        i2c_master_write_byte(cmd, addr_reg_data | I2C_AUTO_INCREMENT, I2C_MASTER_ACK_ENABLE);
    }
    else
    {
        i2c_master_write_byte(cmd, addr_reg_data, I2C_MASTER_ACK_ENABLE);
    }
    
    i2c_master_stop(cmd);

    i2c_master_cmd_begin(machine_i2c, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    esp_err_t err_read = i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ_BIT, I2C_MASTER_ACK_ENABLE);
    
    if (err_read != ESP_OK)
    {
        ESP_LOGE("I2C", "Barramento i2c com erro");
        return ESP_FAIL;
    }
    
    if(len > 1 )
    {
        i2c_master_read(cmd, bufp, len - 1, I2C_MASTER_ACK_ENABLE);
    }

    i2c_master_read_byte(cmd, bufp + len-1, I2C_MASTER_ACK_DISABLE);

    i2c_master_stop(cmd);

    i2c_master_cmd_begin(machine_i2c, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);



    return ESP_OK;
}

/**
 * @brief escrita em um endereço i2c
 * @param machine_i2c qual maquina i2c sera utilizada
 * @param address endereço i2c do escravo
 * @param addr_reg_data endereço do registrador que será acessado
 * @param word dado que será escrito
 * @return esp_err_t
 */
esp_err_t i2c_write(i2c_port_t machine_i2c, uint8_t address, uint8_t addr_reg_data, uint8_t word)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    esp_err_t err_write = i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE_BIT, I2C_MASTER_ACK_ENABLE);
    
    if (err_write != ESP_OK)
    {
        ESP_LOGE("I2C", "Barramento i2c com erro");
        return ESP_FAIL;
    }

    i2c_master_write_byte(cmd, addr_reg_data, I2C_MASTER_ACK_ENABLE);
    i2c_master_write_byte(cmd, word, I2C_MASTER_ACK_ENABLE);
    i2c_master_stop(cmd);

    i2c_master_cmd_begin(machine_i2c, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ESP_OK;
}

esp_err_t i2c_write_mcp(i2c_port_t machine_i2c, uint8_t address, uint8_t addr_reg_data, uint8_t word)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    esp_err_t err_write = i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE_BIT, I2C_MASTER_ACK_ENABLE);
    
    if (err_write != ESP_OK)
    {
        ESP_LOGE("I2C", "Barramento i2c com erro");
        return ESP_FAIL;
    }

    i2c_master_write_byte(cmd, addr_reg_data, I2C_MASTER_ACK_ENABLE);
    i2c_master_write_byte(cmd, word, I2C_MASTER_ACK_ENABLE);
    i2c_master_stop(cmd);

    i2c_master_cmd_begin(machine_i2c, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ESP_OK;
}

/**
 * @brief 
 * 
 */
struct i2c_controller_t *i2c_controller_instance()
{
    if (i2c_controller.i2c_init == 0)
    {
        i2c_controller = (struct i2c_controller_t){
            .i2c_init = &i2c_init,
            .i2c_read_8b = &i2c_read_8b,
            .i2c_read_8b_mcp = &i2c_read_8b_mcp,
            .i2c_read_16b = &i2c_read_16b,
            .i2c_read_bytes = &i2c_read_bytes,
            .i2c_write = &i2c_write,
            .i2c_write_mcp = &i2c_write_mcp,
        };
        i2c_init();
    }
    return &i2c_controller;
    
}