#include "eeprom.h"

#define I2C_MASTER_SCL_IO_EEP   21
#define I2C_MASTER_SDA_IO_EEP   22
#define I2C_MASTER_NUM  I2C_NUM_1
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0

struct eeprom_driver_data_t eeprom_driver_data;
struct eeprom_driver_t eeprom_driver;


void eeprom_init() //é iniciada pelo mcp, usa as mesmas gpio, 21 e 22.
{
    struct i2c_controller_t *i2c_controller = i2c_controller_instance();
}

/**
 * @brief escrita de bytes entre páginas na EEPROM (max 256 bytes)
 * @param deviceaddress endereço da eeprom 0x50
 * @param eeaddress endereço inicial de escrita 0x00
 * @param data dado que será escrito
 * @param size o tamanho da escrita
 * @return ESP_OK
 */
esp_err_t eeprom_write_between_pages(uint8_t deviceaddress, uint32_t eeaddress, uint8_t* data, size_t size)
{
    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd;

    cmd = i2c_cmd_link_create();
    vTaskDelay(20/portTICK_PERIOD_MS);

    uint8_t fist_part = EEPROM_PAGE_SIZE - (eeaddress%EEPROM_PAGE_SIZE);
    uint8_t last_part = size - fist_part;
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (deviceaddress << 1) | EEPROM_WRITE_ADDR | ((eeaddress>>15)&0b10), I2C_MASTER_ACK_ENABLE); //conferir 14?
    i2c_master_write_byte(cmd, eeaddress>>8, I2C_MASTER_ACK_ENABLE);
    i2c_master_write_byte(cmd, eeaddress&0xFF, I2C_MASTER_ACK_ENABLE);
    
    i2c_master_write(cmd, data, fist_part, I2C_MASTER_ACK_ENABLE);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000/portTICK_PERIOD_MS);
    if (ret != ESP_OK) return ret;
    i2c_cmd_link_delete(cmd);


    eeaddress += fist_part;
    
    vTaskDelay(10/portTICK_PERIOD_MS);
    cmd = i2c_cmd_link_create();
    vTaskDelay(20/portTICK_PERIOD_MS);
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (deviceaddress << 1) | EEPROM_WRITE_ADDR | ((eeaddress>>15)&0b10), I2C_MASTER_ACK_ENABLE); //conferir 14?
    i2c_master_write_byte(cmd, eeaddress>>8, I2C_MASTER_ACK_ENABLE);
    i2c_master_write_byte(cmd, eeaddress&0xFF, I2C_MASTER_ACK_ENABLE);

    i2c_master_write(cmd, data+fist_part,last_part , I2C_MASTER_ACK_ENABLE);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000/portTICK_PERIOD_MS);
    if (ret != ESP_OK) return ret;
    i2c_cmd_link_delete(cmd);

    return ret;
}

/**
 * @brief escrita de 1 byte na eeprom
 * @param deviceaddress endereço da eeprom 0x50
 * @param eeaddress endereço inicial de escrita 0x00
 * @param byte o byte que será escrito
 * @return eeprom_write com size 1
 */
esp_err_t eeprom_write_byte(uint8_t deviceaddress, uint16_t eeaddress, uint8_t byte) {
    return eeprom_write(deviceaddress, eeaddress, &byte, 1);
}

/**
 * @brief escrita de paginas
 * @param deviceaddress endereço da eeprom 0x50
 * @param eeaddress endereço inicial de escrita 0x00
 * @param data o que será escrito 
 * @param size o tamanho da escrita
 * @return ESP_OK
 */
esp_err_t eeprom_write(uint8_t deviceaddress, uint32_t eeaddress, uint8_t* data, size_t size) {
    
    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd;
    // i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    //caso queira impedir sobrescrição de dados escritos, descomentar essa area.

    /*i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (deviceaddress << 1) | EEPROM_WRITE_ADDR | ((eeaddress>>15)&0b10), I2C_MASTER_ACK_ENABLE);
    i2c_master_write_byte(cmd, eeaddress >> 8, I2C_MASTER_ACK_ENABLE);
    i2c_master_write_byte(cmd, eeaddress & 0xFF, I2C_MASTER_ACK_ENABLE);

    int first_write_size = ( (EEPROM_PAGE_SIZE - 1) - eeaddress % (EEPROM_PAGE_SIZE - 1) ) + 1;
    if (eeaddress % (EEPROM_PAGE_SIZE-1) == 0 && eeaddress != 0) {
        first_write_size = 1;
    }
    if (size <= first_write_size) {
        i2c_master_write(cmd, data, size, 1);
    } 
    else {
        i2c_master_write(cmd, data, first_write_size, 1);
        size -= first_write_size;
        eeaddress += first_write_size;
        i2c_master_stop(cmd);
        
        ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000/portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
    
        if (ret != ESP_OK) { 
        return ret;
    }*/
    
    for(uint32_t i = 0; i<size; i+=EEPROM_PAGE_SIZE)
    {
        // // 2ms delay period to allow EEPROM to write the page
        // // buffer to memory.
        if( ((eeaddress%EEPROM_PAGE_SIZE) + size) > EEPROM_PAGE_SIZE)
        {
            ret = eeprom_write_between_pages(deviceaddress, eeaddress+i, data+i, size);
            if (ret != ESP_OK) return ret;
        }
        else
        {
            cmd = i2c_cmd_link_create();

            vTaskDelay(20/portTICK_PERIOD_MS);
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (deviceaddress << 1) | EEPROM_WRITE_ADDR | ((eeaddress>>15)&0b10), I2C_MASTER_ACK_ENABLE); //conferir 14?
            
            i2c_master_write_byte(cmd, eeaddress>>8, I2C_MASTER_ACK_ENABLE);
            i2c_master_write_byte(cmd, eeaddress&0xFF, I2C_MASTER_ACK_ENABLE);
            if (size-i <= EEPROM_PAGE_SIZE) {
                i2c_master_write(cmd, data+i, size-i, I2C_MASTER_ACK_ENABLE);
                eeaddress += size-i;
            } else {
                i2c_master_write(cmd, data+i, EEPROM_PAGE_SIZE, I2C_MASTER_ACK_ENABLE);
                eeaddress += EEPROM_PAGE_SIZE;
            }
            i2c_master_stop(cmd);
            ret = i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000/portTICK_PERIOD_MS);
            i2c_cmd_link_delete(cmd);
            if (ret != ESP_OK) return ret;
        } 
    }
    return ret;

}
/**
 * @brief leitura de 1 byte
 * @param deviceaddress endereço da eeprom 0x50
 * @param eeaddress endereço inicial de escrita 0x00
 * @param byte retorna o valor lido em uma variavel
 */
esp_err_t eeprom_read_byte(uint8_t deviceaddress, uint16_t eeaddress, uint8_t *byte) {
    esp_err_t ret;
    vTaskDelay(20/portTICK_PERIOD_MS);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (deviceaddress << 1) | EEPROM_WRITE_ADDR, 1);
    i2c_master_write_byte(cmd, eeaddress << 8, 1);
    i2c_master_write_byte(cmd, eeaddress & 0xFF, 1);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (deviceaddress << 1) | EEPROM_READ_ADDR, 1);

    i2c_master_read_byte(cmd, byte, 1);
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000/portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

/**
 * @brief leitura de bytes
 * @param deviceaddress endereço da eeprom 0x50
 * @param eeaddress endereço inicial de escrita 0x00
 * @param data ponteiro para resultado da leitura
 * @param size tamanho para leitura de resultado
 */
esp_err_t eeprom_read(uint8_t deviceaddress, uint32_t eeaddress, uint8_t* data, size_t size) {
    esp_err_t ret;
    vTaskDelay(20/portTICK_PERIOD_MS);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (deviceaddress<<1)|EEPROM_WRITE_ADDR| ((eeaddress>>15)&0b10), 1);
        i2c_master_write_byte(cmd, eeaddress>>8, 1); //pode ser aqui é >> não <<
        i2c_master_write_byte(cmd, eeaddress&0xFF, 1);
        

        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (deviceaddress<<1)|EEPROM_READ_ADDR| ((eeaddress>>15)&0b10), 1);
        if (size > 1) {
            i2c_master_read(cmd, data, size, 0);
        }
        i2c_master_read_byte(cmd, data+size, 1);
        i2c_master_stop(cmd);
    // }
    ret = i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000/portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

struct eeprom_driver_t *eeprom_driver_instance()
{
    if (eeprom_driver.init == 0)
    {
        eeprom_driver = (struct eeprom_driver_t){
            .init = &eeprom_init,
            .read_byte = &eeprom_read_byte,
            .write_byte = &eeprom_write_byte,
            .read_buffer = &eeprom_read,
            .write_buffer = &eeprom_write,
        };
    }
    return &eeprom_driver;
}
