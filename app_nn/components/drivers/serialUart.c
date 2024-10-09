#include "serialUart.h"
#include "time.h"
#include "storage.h"
#include <sys/time.h>
#include "esp_ota_ops.h"
#include "RSA.c"

#define BUF_SIZE 1024
#define INPUT_LENGTH 3
#define INPUT_LENGTH_RES 4
#define MBEDTLS_CIPHER_MODE_CFB

struct serialUart_driver_data_t *serialUart_driver_data = &serialUart_data_driver;
struct serialUart_driver_t serialUart_driver;

mbedtls_aes_context aes;

unsigned char key[16] = {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0X6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70}; //"abcdefghijklmnop";
unsigned char key_iv[16] = {};
unsigned char key_iv_buffer[16] = {0};

unsigned char output_serial[INPUT_LENGTH] = {0};
unsigned char input_serial[INPUT_LENGTH] = {0};

esp_err_t serial_uart_write(char *buffer_tx, uint32_t size);

typedef struct __attribute__((__packed__)){
    uint32_t firm_size;
    uint32_t checksum;
}packet_t;

void header_send(char type[3 + 1], unsigned char b1, unsigned char b2, unsigned char key_iv[16])
{
    unsigned char header[5],
                  headerEncrypt[5],
                  buffer_uart[16 + 5];


    header[0] = type[0];
    header[1] = type[1];
    header[2] = type[2];
    header[3] = b1;
    header[4] = b2;

    for (size_t i = 0; i < 16; i++){
        buffer_uart[i] = key_iv_buffer[i];
    }

    mbedtls_aes_crypt_cfb8(&aes, MBEDTLS_AES_ENCRYPT, 5, key_iv, header, headerEncrypt);

    for (size_t i = 0; i < 5; i++){
        buffer_uart[i+16] = (unsigned char) headerEncrypt[i];
    }

    serial_uart_write((char *)buffer_uart, 16 + 5);
}

void mass_send(uint8_t max_add, 
               uint8_t* response_input,  
               uint8_t* response_output, 
               uint16_t size,
               uint32_t *address_recent_log, 
               unsigned char *buffer_uart, 
               struct storage_device_t *storage_eeprom)
{   
    uint16_t updated_position;
    for (uint8_t i = 1; i <= max_add; i++)
    {
        updated_position = 0;
        storage_eeprom->load_memory(MAX_SIZE, *address_recent_log, response_output);
        
        //Inverte o vetor
        for (int last_log = size - 10; last_log >= 0; last_log -= 10){
            for(int j = 0; j < 10; j++)
                response_input[updated_position + j] = response_output[last_log + j];
            updated_position += 10;
        }

        mbedtls_aes_crypt_cfb8(&aes, MBEDTLS_AES_ENCRYPT, size, key_iv, response_input, response_output);

        memcpy(buffer_uart, response_output, size * sizeof(char));

        size_t a = 0;
        for (; (a + 2000) < size; a += 2000)
            serial_uart_write((char *)(buffer_uart+a), 2000);
        serial_uart_write((char *)(buffer_uart+ a), size - a);

        //Evitar address negativo
        if (i < max_add)
            *address_recent_log -= size;
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}

void minor_send(uint16_t size_remaining_logs, 
                uint8_t* response_input, 
                uint8_t* response_output, 
                uint32_t address,
                unsigned char *buffer_uart,
                struct storage_device_t *storage_eeprom)
{
    
    uint16_t updated_position = 0;  
    storage_eeprom->load_memory(size_remaining_logs, address, response_output);

    for (int last_log = size_remaining_logs - 10; last_log >= 0; last_log -= 10){
        for(int j = 0; j < 10; j++)
            response_input[updated_position + j] = response_output[last_log + j];
        updated_position += 10;
    }

    mbedtls_aes_crypt_cfb8(&aes, MBEDTLS_AES_ENCRYPT, size_remaining_logs, key_iv, response_input, response_output);

    memcpy(buffer_uart, response_output, size_remaining_logs * sizeof(char));
            
    size_t i = 0;
    for (; (i + 2000) <  size_remaining_logs; i += 2000)
        serial_uart_write(((char *)buffer_uart+i), 2000);
    serial_uart_write((char *)(buffer_uart+ i), size_remaining_logs - i);
}

void response_c01()
{
    unsigned char response_output[5] = {0};
    unsigned char response_input[5] = {0};
    unsigned char buffer_uart[21] = {0};

    response_input[0] = 'R';
    response_input[1] = '0';
    response_input[2] = '1';

    response_input[3] = serialUart_driver_data->predict_model;
    response_input[4] = 0;
    printf("Predict model - %d \n", serialUart_driver_data->predict_model);
    
    
    for (size_t i = 0; i < 16; i++)
    {
        key_iv[i] = key_iv_buffer[i];
        buffer_uart[i] = key_iv_buffer[i];
    }

    mbedtls_aes_crypt_cfb8(&aes, MBEDTLS_AES_ENCRYPT, INPUT_LENGTH_RES, key_iv, response_input, response_output);
    
    for (size_t i = 0; i < 5; i++)
    {
        buffer_uart[i+16] = response_output[i];
    }
    

    serial_uart_write((char *)buffer_uart, 21);

}

void response_c02()
{
    unsigned char buffer_uart[21] = {0};
    unsigned char response_output[5] = {0};
    unsigned char response_input[5] = {0};

    response_input[0] = 'R';
    response_input[1] = '0';
    response_input[2] = '2';
    response_input[4] = 0;
    
    printf("Resultado da consulta ao autoteste -> ");
    for (size_t i = 0; i < 8; i++)
    {
        response_input[3] |= serialUart_driver_data->res_autoteste[i] << i;
        printf("%d ", serialUart_driver_data->res_autoteste[i]);
    }
    printf("\n");
    
    for (size_t i = 0; i < 16; i++)
    {
        key_iv[i] = key_iv_buffer[i];
        buffer_uart[i] = key_iv_buffer[i];
    }
    
    mbedtls_aes_crypt_cfb8(&aes, MBEDTLS_AES_ENCRYPT, INPUT_LENGTH_RES, key_iv, response_input, response_output);
    
    for (size_t i = 0; i < 5; i++)
    {
        buffer_uart[i+16] = response_output[i];
    }
    
    serial_uart_write((char *)buffer_uart,21);

}

void response_c03()
{
    unsigned char buffer_uart[21] = {0};
    unsigned char response_output[5] = {0};
    unsigned char response_input[5] = {0};

    response_input[0] = 'R';
    response_input[1] = '0';
    response_input[2] = '3';
    response_input[4] = 0;

    struct sensors_t *sensors_guardian = sensors_instance();
    sensors_guardian->sensors_autoteste();
   
    printf("Resultado do autoteste -> ");
    for (size_t i = 0; i < 8; i++)
    {
        response_input[3] |= serialUart_driver_data->res_autoteste[i] << i;
        printf("%d ", serialUart_driver_data->res_autoteste[i]);
    }
    printf("\n");
    
    for (size_t i = 0; i < 16; i++)
    {
        key_iv[i] = key_iv_buffer[i];
        buffer_uart[i] = key_iv_buffer[i];
    }

    mbedtls_aes_crypt_cfb8(&aes, MBEDTLS_AES_ENCRYPT, INPUT_LENGTH_RES, key_iv, response_input, response_output);
    
    for (size_t i = 0; i < 5; i++)
    {
        buffer_uart[i+16] = response_output[i];
    }
    
    serial_uart_write((char *)buffer_uart,21);

}

void response_c04()
{
    unsigned char buffer_uart[21] = {0};
    unsigned char response_output[5] = {0};
    unsigned char response_input[5] = {0};

    response_input[0] = 'R';
    response_input[1] = '0';
    response_input[2] = '4';

    response_input[3] = serialUart_driver_data->predict_model_history;
    response_input[4] = 0;
    printf("Predict model history- %d ", serialUart_driver_data->predict_model_history);
    serialUart_driver_data->predict_model_history = 0;
    
    
    for (size_t i = 0; i < 16; i++)
    {
        key_iv[i] = key_iv_buffer[i];
        buffer_uart[i] = key_iv_buffer[i];
    }

    mbedtls_aes_crypt_cfb8(&aes, MBEDTLS_AES_ENCRYPT, INPUT_LENGTH_RES, key_iv, response_input, response_output);
    
    for (size_t i = 0; i < 4; i++)
    {
        buffer_uart[i+16] = response_output[i];
    }
    
    serial_uart_write((char *)buffer_uart,21);

}

void response_c05()
{
    char TAG[13] = "Att firmware";
    esp_err_t err;
    esp_ota_handle_t update_handle = 0 ;
    uint32_t binary_file_length = 0, checksum = 0, checksum_recieve_data;
    const esp_partition_t *update_partition = NULL;
    
    uint8_t* ota_write_data = (uint8_t*) malloc(BUF_SIZE+1);
    packet_t packet;

    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x", update_partition->subtype, update_partition->address);
    
    uart_write_bytes(UART_NUM_1, "OK", 2);
    

    for(;;)
    {
        int rxBytes = uart_read_bytes(UART_NUM_1, &packet, sizeof(packet), 1000 / portTICK_PERIOD_MS);
        
        ESP_LOGI(TAG, "Esperando firmware");
        if (rxBytes > 0) {
            err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                esp_ota_abort(update_handle);
                esp_restart();
            }
            
            ESP_LOGI(TAG, "esp_ota_begin succeeded");
            printf("Size_firm: %d\n",packet.firm_size);
            uart_write_bytes(UART_NUM_1, "OBP", 3);

            for(;;)
            {
                rxBytes = uart_read_bytes(UART_NUM_1, ota_write_data, 1024, 1000 / portTICK_PERIOD_MS);  
                if (rxBytes > 0) {
                    checksum = 0;
                    if(rxBytes>1023){
                        for(int i = 4; i<1024; i++)
                            checksum += ota_write_data[i];
                        binary_file_length += rxBytes-4;
                    }
                    else
                    {
                        for(int i = 4; i<rxBytes;i++)
                            checksum += ota_write_data[i];
                        binary_file_length += rxBytes;
                    }
                    //Os 4 primeiros bytes são checksum do pacote
                    checksum_recieve_data = *(uint32_t*)ota_write_data;

                    if (checksum!=checksum_recieve_data)
                    {
                        esp_ota_abort(update_handle);
                        uart_write_bytes(UART_NUM_1, "CSF", 3);
                        esp_restart();
                    }

                    err = esp_ota_write( update_handle, (const void *) (ota_write_data+4), rxBytes-4);
                    if (err != ESP_OK) {
                        esp_ota_abort(update_handle);
                        uart_write_bytes(UART_NUM_1, "OWF", 3);
                        esp_restart();
                        
                    }
                    
                    //ESP_LOGI(TAG, "Written image length %d", binary_file_length);
                    if (binary_file_length >= packet.firm_size)
                    {
                        uart_write_bytes(UART_NUM_1, "R05", 3);
                        break;
                    }
                    uart_write_bytes(UART_NUM_1, "R05", 3);
                } 
            
            }
        if (binary_file_length >= packet.firm_size)
            break;
        }       
    }

    ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        } else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
        esp_restart();
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        esp_restart();
    }

    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
    
}

void response_c06()
{
    unsigned char output_serial_timestamp[4] = {0};
    unsigned char input_serial_timestamp[4] = {0};
    
    uint8_t* data_timestamp = (uint8_t*) malloc(21);
    //É preciso de um delay de 0.1 s no envio do tt para que o sistema esteja pronto para ler
    uart_write_bytes(UART_NUM_1, "OK", 2);
    const int rxBytes_timestamp = uart_read_bytes(UART_NUM_1, data_timestamp, 20, 2000 / portTICK_PERIOD_MS);
        if (rxBytes_timestamp > 0) {
            data_timestamp[rxBytes_timestamp] = 0;
            ESP_LOGI("RX_TASK_TAG", "Read %d bytes: '%s'", rxBytes_timestamp, data_timestamp);
            
            
            for (size_t i = 0; i < 16; i++)
            {
                key_iv[i] = data_timestamp[i];
                key_iv_buffer[i] = data_timestamp[i];
                printf("%x ", data_timestamp[i]);
            }
            printf("\n");

            for (size_t i = 16; i < 20; i++)
            {
                input_serial_timestamp[i - 16] = data_timestamp[i];
                printf("%d ", input_serial_timestamp[i - 16]);
            }
            printf("\n");

            mbedtls_aes_crypt_cfb8(&aes, MBEDTLS_AES_DECRYPT, 4, key_iv, input_serial_timestamp, output_serial_timestamp);
            
            printf("decrypt: \n");
            for (size_t i = 0; i < 4; i++)
            {
                printf("%d ", output_serial_timestamp[i]);
            }
            printf("\n");

            uart_flush_input(UART_NUM_1);
            
            uint32_t *timestamp_esp = (uint32_t*)output_serial_timestamp;
            printf("ts recebido- %d \n", *timestamp_esp);
            struct timeval tv_now;
            tv_now.tv_sec = *timestamp_esp;

            settimeofday(&tv_now, NULL);
            if (time(NULL) != *timestamp_esp) //settimeofday tem um erro de 1073, Realizando novamente ele salva o tempo correto.
            {
                vTaskDelay(1000/portTICK_PERIOD_MS);
                struct timeval tv_now;
                tv_now.tv_sec = *timestamp_esp+1;
                settimeofday(&tv_now, NULL);
            }            

            printf("ts interno atualizado - %ld \n", time(NULL));
            uart_write_bytes(UART_NUM_1, "TOK", 3);

        }
        free(data_timestamp);
}

void response_c07()
{
    struct storage_device_t *storage_eeprom = storage_device_instance();
    extern struct storage_data_t storage_data;

    uint32_t address,
             size_logs_disp;
    uint16_t num_logs_disp,
             updated_position;
    uint8_t qnt_returned,
            remaining_logs,
            max_add,
            *response_output = (uint8_t*) malloc(MAX_SIZE + 1),
            *response_input =  (uint8_t*) malloc(MAX_SIZE + 1);
    unsigned char buffer_uart[MAX_SIZE + 1];

    qnt_returned = storage_eeprom->check_num_logs(&num_logs_disp, 200);
    size_logs_disp = num_logs_disp*SIZE_LOG;

    address = 0;
    if (qnt_returned == ZERO_LOGS){
        
        for (size_t i = 0; i < 16; i++){
            key_iv[i] = key_iv_buffer[i];
        }
        header_send("R07", 0, 0, key_iv);
        
    }

    if (qnt_returned == DEFAULT){

        //Enviando o cabeçalho : R07(Quantidade de logs)
        for (size_t i = 0; i < 16; i++){
            key_iv[i] = key_iv_buffer[i];
        }
        header_send("R07", (unsigned char) (num_logs_disp>>8), (unsigned char) num_logs_disp, key_iv);
        max_add = size_logs_disp/MAX_SIZE;
 
        if (max_add > 0){
            address = storage_data.pos_recent_log - MAX_SIZE + ADDRESS_FIST_LOG;
            mass_send(max_add, response_input, response_output, MAX_SIZE, &address, buffer_uart, storage_eeprom);
        } 

        remaining_logs = (size_logs_disp % MAX_SIZE)/SIZE_LOG; 
        if (remaining_logs){
            if (address == 0)
                address = storage_data.pos_recent_log - (remaining_logs * SIZE_LOG) + ADDRESS_FIST_LOG;
            else
                address -= remaining_logs*SIZE_LOG;
            minor_send(remaining_logs * SIZE_LOG, response_input, response_output, address, buffer_uart, storage_eeprom);   
        }
          
    }
    
    if (qnt_returned == PART_LOGS_FULL){

        uint16_t size_final_logs = 0;
    
        for (size_t i = 0; i < 16; i++){
            key_iv[i] = key_iv_buffer[i];
        }
        header_send("R07", (unsigned char) (num_logs_disp>>8), (unsigned char) num_logs_disp, key_iv);

        if (num_logs_disp > storage_data.pos_recent_log/SIZE_LOG){
            size_final_logs = num_logs_disp*SIZE_LOG - (storage_data.pos_recent_log);
            size_logs_disp -= size_final_logs;
        }

        address = 0;
        max_add = size_logs_disp/MAX_SIZE;
        if (max_add > 0){
            address = storage_data.pos_recent_log - MAX_SIZE + ADDRESS_FIST_LOG;
            mass_send(max_add, response_input, response_output, MAX_SIZE, &address, buffer_uart, storage_eeprom);
        } 

        remaining_logs = (size_logs_disp % MAX_SIZE)/SIZE_LOG; 
        if (remaining_logs){
            if (address == 0)
                address = storage_data.pos_recent_log - (remaining_logs * SIZE_LOG) + ADDRESS_FIST_LOG;
            else
                address -= remaining_logs*SIZE_LOG;
            minor_send(remaining_logs * SIZE_LOG, response_input, response_output, address, buffer_uart, storage_eeprom);
        }

        if (num_logs_disp > storage_data.pos_recent_log/SIZE_LOG)
        {
            max_add = size_final_logs/MAX_SIZE;
            if(max_add){
                address = ADDRESS_LAST_LOG - MAX_SIZE + ADDRESS_FIST_LOG;
                mass_send(max_add, response_input, response_output, MAX_SIZE, &address, buffer_uart, storage_eeprom); 
            }
     
            remaining_logs = (size_final_logs % MAX_SIZE)/SIZE_LOG;
            if(remaining_logs){
                if (address == 0)
                address = ADDRESS_LAST_LOG - (remaining_logs * SIZE_LOG) + ADDRESS_FIST_LOG;
            else
                address -= remaining_logs*SIZE_LOG;
            minor_send(remaining_logs * SIZE_LOG, response_input, response_output, address, buffer_uart, storage_eeprom);
            }
        }
    }

    ESP_LOGI("RX_TASK_TAG", "Send %d Logs",num_logs_disp);
    free(response_input);
    free(response_output);  
}

static void serial_uart_read(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE+1);
    
    while (true)
    {
        // printf("Tentando ler: %d\n",heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, 19, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);

            for (size_t i = 0; i < 16; i++)
            {
                key_iv[i] = data[i];
                key_iv_buffer[i] = data[i];
                printf("%x ", data[i]);
            }
            printf("\n");

            for (size_t i = 16; i < 19; i++)
            {
                input_serial[i - 16] = data[i];
                printf("%d ", input_serial[i - 16]);
            }
            printf("\n");

            mbedtls_aes_crypt_cfb8(&aes, MBEDTLS_AES_DECRYPT, INPUT_LENGTH, key_iv, input_serial, output_serial);
            
            printf("decrypt: \n");
            for (size_t i = 0; i < INPUT_LENGTH; i++)
            {
                printf("%d ", output_serial[i]);
            }
            printf("\n");
            
            ESP_LOGI("cfb128", "%s", output_serial);
            uart_flush_input(UART_NUM_1);

            if (output_serial[0] == 'C')
            {
                if (output_serial[1] == '0')
                {
                    if (output_serial[2] == '1')
                    {
                        printf("Req C01 \n");
                        response_c01();
                    }
                    else if (output_serial[2] == '2')
                    {
                        printf("Req C02 \n");
                        response_c02();
                    }
                    else if (output_serial[2] == '3')
                    {
                        printf("Req C03 \n");
                        response_c03();
                    }
                    else if (output_serial[2] == '4')
                    {
                        printf("Req C04 \n");
                        response_c04();
                    }
                    else if (output_serial[2] == '5')
                    {
                        printf("Req C05 \n");
                        response_c05();
                    }
                    else if (output_serial[2] == '6')
                    {
                        printf("Req C06 \n");
                        response_c06();
                    }
                    else if (output_serial[2] == '7')
                    {
                        printf("Req C07 \n");
                        response_c07();
                    }
                    else if (output_serial[2] == '8')
                    {
                         RSA_test();
                    }
                    
                }
            }
        }
    }

    free(data);
    
}

/**
 * @brief Esta função inicializa a UART2 responsável pela comunicação RS232
 * 
 * @return esp_err_t 
 */
esp_err_t serial_uart_init(void)
{
    const uart_port_t uart_num = UART_NUM_1;
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    // Configure UART parameters
    esp_err_t err;

    err = uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);

    if(err != ESP_OK)
    {
        ESP_LOGE("UART1", "Erro de instalação do driver");
        return ESP_FAIL;
    }

    err = uart_param_config(uart_num, &uart_config);
    
    if (err != ESP_OK)
    {
        ESP_LOGE("UART1", "Erro ao configurar os parâmetros da UART");
        return ESP_FAIL;
    }

    // Set UART pins(TX: IO4, RX: IO5, RTS: IO18, CTS: IO19)
    err = uart_set_pin(UART_NUM_1, 10, 9, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    if (err != ESP_OK)
    {
        ESP_LOGE("UART1", "Erro ao configurar os pinos da UART");
        return ESP_FAIL;
    }

    ESP_LOGI("UART1", "UART iniciou");


    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 128);
    /* mbedtls_aes_free(&aes); */

    xTaskCreate(serial_uart_read, "serial_uart_read", 6154 + 512, NULL, 11, NULL);

    return ESP_OK;
}

esp_err_t serial_uart_write(char *buffer_tx, uint32_t size)
{
    // Write data to UART.
    // printf("\nSTRLEN: %d\n", size);
    // printf("buffer_tx :%p\n",buffer_tx);
    uart_write_bytes(UART_NUM_1, (char*)buffer_tx, size);
    return ESP_OK;

}

struct serialUart_driver_t *serialUart_driver_instance()
{
    if (serialUart_driver.serial_uart_init == 0)
    {
        serialUart_driver = (struct serialUart_driver_t){
            .serial_uart_init = &serial_uart_init,
            .serial_uart_write = &serial_uart_write,
        };
    }
    
    return &serialUart_driver;
}
