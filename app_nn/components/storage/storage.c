#include "storage.h"
#include "freertos/semphr.h"

struct storage_data_t storage_data;
struct storage_device_t storage_device;

char TAG[9] = "Storage";

//debug

void print_memory_info(){
    printf("storage_data.pos_old_log: %u\n",storage_data.pos_old_log);
    printf("storage_data.pos_recent_log: %u\n", storage_data.pos_recent_log);
    printf("storage_data.manager_eeprom: %u\n", storage_data.manager_eeprom);
    printf("Memory full: %u\n", storage_data.memory_full);
    printf("Flag is inverted: %u\n", storage_data.flag_is_inverted);
}

//task

static void storage_log_task(void *pvParameters)
{
    struct storage_device_t *storage_eeprom = storage_device_instance();
    uint32_t log_data[4], checksum;
    uint8_t id_flag_log, info;
    time_t tt;
    uint8_t * response = (uint8_t *)malloc(11);
    for(;;)
    {
        if (uxQueueMessagesWaiting(storage_data.save_logs_queue)&&
            xSemaphoreTake(storage_data.xSemaphore_eeprom, 0))
        {
            xQueueReceive(storage_data.save_logs_queue, &log_data, 0);
            
            // printf("Salving\n"); 
            // printf("storage_data.manager_eeprom: %d\n",storage_data.manager_eeprom);      
            id_flag_log = (uint8_t) log_data[0];
            tt =                    log_data[1];
            info =        (uint8_t) log_data[2];
            checksum =              log_data[3];
            storage_eeprom->save_data(&id_flag_log, 1);
            storage_eeprom->save_data(&tt, 4);
            storage_eeprom->save_data(&info, 1);
            storage_eeprom->save_data(&checksum,4);
            // printf("tt: %ld\n",tt);
            // printf("id_flag_log: %d \n", id_flag_log);
            // printf("tt: %d \n", log_data[1]);
            // printf("info: %d  \n", info );
            // printf("checksum: %d  \n\n\n\n", log_data[3]);
             
            // free(response);
            #ifdef CHECK_SAVE
                uint8_t ldc[10], fail; //ldc -> log data check
                uint32_t ccs, ctt;
                for(uint8_t check = 0; check<1; check++)
                {
                    vTaskDelay(10/portTICK_PERIOD_MS);
                    fail = 0;
                    storage_eeprom->load_data(ldc, SIZE_LOG, storage_data.manager_eeprom - SIZE_LOG);

                    ccs = (ldc[9]<<24 | ldc[8]<<16 | ldc[7]<<8 | ldc[6]), 
                    ctt = (ldc[4]<<24 | ldc[3]<<16 | ldc[2]<<8 | ldc[1]);
                
                    if( (id_flag_log != ldc[0]))
                    {
                        fail = 1;
                        printf("id_flag_log: %d  |   %d\n", id_flag_log, ldc[0]);
                        // eeprom_write(ADDRESS_EEPROM, storage_data.manager_eeprom-10, &id_flag_log, 1);
                    }

                    if(log_data[1] != ctt )
                    {
                        fail = 1;
                        printf("tt: %d  |  %d\n", log_data[1], ctt);
                    }

                    if(info != ldc[5])
                    {
                        fail = 1;
                        printf("info: %d  | %d\n", info, ldc[5] );
                        // eeprom_write(ADDRESS_EEPROM, storage_data.manager_eeprom-5, &info, 1);
                    }
                    
                    if(log_data[3] != ccs)
                    {
                        fail = 1;
                        // uint8_t *c = (uint8_t*) &log_data[3];
                        printf("checksum: %d  | %d\n", checksum, ccs );
                    }

                    if(!fail)
                    {
                        // printf("passou\n");
                        break;
                    }
                    if(check == 1)
                        printf("Erro no log do endereço: %d\n",storage_data.manager_eeprom-10);
                    
                }
            #endif


            // uint8_t * response = (uint8_t *)malloc(11);
            // storage_eeprom->load_data(response, SIZE_LOG, storage_data.manager_eeprom - SIZE_LOG);
            // for (size_t i = 0; i < 10; i++)
            // {
            //     printf("%d \n", response[i]);
            // }
            // free(response);
            // printf("End %d\n",heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
            // printf("Dando da task save\n");
            xSemaphoreGive(storage_data.xSemaphore_eeprom);       
        }
        vTaskDelay(50/portTICK_PERIOD_MS);
    }
}

//init

void storage_struct_init(uint8_t valid_info[10]){

    storage_data.pos_old_log |= ( valid_info[3]<<24 | valid_info[2]<<16 | valid_info [1]<<8 | valid_info[0]);
    storage_data.pos_recent_log |= ( valid_info[7]<<24 | valid_info[6]<<16 | valid_info [5]<<8 | valid_info[4]);
    
    storage_data.memory_full = valid_info[8];

    if (!storage_data.memory_full) 
        storage_data.manager_eeprom = storage_data.pos_recent_log + SIZE_LOG; 
    else
        storage_data.manager_eeprom = storage_data.pos_old_log;
    
    eeprom_read(ADDRESS_EEPROM, storage_data.pos_recent_log, valid_info, 10);

    if (!storage_data.memory_full) 
        storage_data.flag_is_inverted = 0;
    else if (FLAG_IS_DEFAULT(valid_info[0]))
        storage_data.flag_is_inverted = 0;
    else if (FLAG_IS_INVERT(valid_info[0]))
        storage_data.flag_is_inverted = 1;
    else
    {
        eeprom_read(ADDRESS_EEPROM, storage_data.pos_recent_log - 10, valid_info, 10);
        if (FLAG_IS_DEFAULT(valid_info[0]))
        storage_data.flag_is_inverted = 0;
        else if (FLAG_IS_INVERT(valid_info[0]))
        storage_data.flag_is_inverted = 1;
    }
            
    #ifdef MEMORY_INFO
        printf("Checagem de memoria Passou\nIniciando das posicoes:\n");
        print_memory_info();
    #endif
}

void storage_managers_init(){
    storage_data.save_logs_queue = xQueueCreate(10, sizeof(uint32_t)* 4);

    storage_data.xSemaphore_eeprom = xSemaphoreCreateBinary();
    xSemaphoreGive(storage_data.xSemaphore_eeprom);

    xTaskCreate(storage_log_task,"storage_log_task", 1*1024, NULL, 10, NULL);
}

esp_err_t eeprom_info_init(){
    // Estrutura -> 4 bytes old_log | 4 bytes most_recent_log | 2 bytes memory full (0,1)
    uint8_t i_log_valid[10], f_log_valid[10], memory_ok = 1;
    eeprom_read(ADDRESS_EEPROM, 0x00, i_log_valid, 10);
    eeprom_read(ADDRESS_EEPROM, EEPROM_FULL_SIZE - 10, f_log_valid, 10);
    
    for (size_t i = 0; i < 10; i++)
    {   
        if (i_log_valid[i] != f_log_valid[i]) 
            memory_ok = 0;
        #ifdef MEMORY_INFO
        printf("%d | %d\n",i_log_valid[i],f_log_valid[i]);
        #endif
    }
          
    //se não, faz a verificação por brute force
    if (memory_ok) {
        storage_struct_init(i_log_valid);
        storage_managers_init();
        ESP_LOGI(TAG, "Dispositivo configurado com sucesso");
        return ESP_OK;
    }
    else{
        printf("Erro: memory not ok\n");
        return ESP_FAIL;
    }
        
}

esp_err_t inversion_flag_update(){
    uint8_t log[SIZE_LOG];
    eeprom_read(ADDRESS_EEPROM, 10, log, SIZE_LOG);
     //checa se o primeiro valor na memória corresponde as flags estabelecidas
    if(FLAG_IS_DEFAULT(log[0])) 
        storage_data.flag_is_inverted = 0; 
    else if(FLAG_IS_INVERT(log[0])) 
        storage_data.flag_is_inverted = 1;
    else{
        return ESP_FAIL;
    }
    return ESP_OK;
}

//Gerenciamento da struct de controle

void save_eeprom_controller(void)
{
    uint32_t a;
    a = 0x00;
    eeprom_write(ADDRESS_EEPROM, a, &storage_data.pos_old_log, 4); 
    a = EEPROM_FULL_SIZE - 10;
    eeprom_write(ADDRESS_EEPROM, a, &storage_data.pos_old_log, 4);
    a = 0x04;
    eeprom_write(ADDRESS_EEPROM, a, &storage_data.pos_recent_log, 4);
    a = EEPROM_FULL_SIZE - 6;
    eeprom_write(ADDRESS_EEPROM, a, &storage_data.pos_recent_log, 4);
    a = 0x08;
    eeprom_write(ADDRESS_EEPROM, a, &storage_data.memory_full, 2);
    a = EEPROM_FULL_SIZE - 2;
    eeprom_write(ADDRESS_EEPROM, a, &storage_data.memory_full, 2);
}

//Gerenciamento da memória EEPROM

void reset_memory()
{
    printf("Resetando memoria\n");
    uint32_t a;
    storage_data.manager_eeprom = 10; 
    storage_data.pos_recent_log = 10;
    storage_data.pos_old_log = 10;
    storage_data.memory_full = 0;
    storage_data.flag_is_inverted = 0;
    a = 0x00;
    eeprom_write(ADDRESS_EEPROM, a, &storage_data.pos_old_log, 4); 
    a = EEPROM_FULL_SIZE - 10;
    eeprom_write(ADDRESS_EEPROM, a, &storage_data.pos_old_log, 4);
    a = 0x04;
    eeprom_write(ADDRESS_EEPROM, a, &storage_data.pos_recent_log, 4);
    a = EEPROM_FULL_SIZE - 6;
    eeprom_write(ADDRESS_EEPROM, a, &storage_data.pos_recent_log, 4);
    a = 0x08;
    eeprom_write(ADDRESS_EEPROM, a, &storage_data.memory_full, 2);
    a = EEPROM_FULL_SIZE - 2;
    eeprom_write(ADDRESS_EEPROM, a, &storage_data.memory_full, 2);

    storage_data.flag_is_inverted = 0;
    storage_data.manager_eeprom = 10;
}

esp_err_t load_memory(uint32_t size_num_logs, uint32_t initial_address ,uint8_t *response)
{
    xSemaphoreTake(storage_data.xSemaphore_eeprom, portMAX_DELAY);
    //O controle do endereço tem que ser feito por fora
        size_t i = 0;
        
        for (; (i + EEPROM_READ_SIZE) <= size_num_logs; i += EEPROM_READ_SIZE)
        {
            eeprom_read(ADDRESS_EEPROM, initial_address + i, response + i, EEPROM_READ_SIZE); 
            vTaskDelay(10/portTICK_PERIOD_MS);
        }

        //Quando 25 passar do total, preenche o restante
        if ( (size_num_logs - i) != 0)
            eeprom_read(ADDRESS_EEPROM, i + initial_address, response + i, (size_num_logs - i));
    
    xSemaphoreGive(storage_data.xSemaphore_eeprom);
    return ESP_OK; 
}

esp_err_t save_data(uint8_t *data, size_t size)
{   
    uint32_t a;
   
    if(storage_data.manager_eeprom >= ADDRESS_LAST_LOG + SIZE_LOG) // não uso os ultimos 10 bytes, para não ter erro na escrita das palavras 50 -> 60 (stop) -> 61
    {   
        storage_data.manager_eeprom = 0x0A;
        storage_data.memory_full = 1;
        a = 0x08;
        eeprom_write(ADDRESS_EEPROM, a, &storage_data.memory_full, 2);
        a = EEPROM_FULL_SIZE - 2;
        eeprom_write(ADDRESS_EEPROM, a, &storage_data.memory_full, 2);
    }
    
    eeprom_write(ADDRESS_EEPROM, storage_data.manager_eeprom, data, size);
    storage_data.manager_eeprom += size;

    if (storage_data.manager_eeprom % SIZE_LOG == 0) //escrita concluida com sucesso
    {
        if (storage_data.memory_full)
        {
            storage_data.pos_old_log = storage_data.manager_eeprom; // o proprio storage_data.manager_eeprom a posicao que estou indicando para ser a próxima alteração  é o log mais antigo
            a = 0x00;
            eeprom_write(ADDRESS_EEPROM, a, &storage_data.pos_old_log, 4);
            a = EEPROM_FULL_SIZE - 10;
            eeprom_write(ADDRESS_EEPROM, a, &storage_data.pos_old_log, 4);
        }
        
        storage_data.pos_recent_log = storage_data.manager_eeprom - SIZE_LOG;
        a = 0x04;
        eeprom_write(ADDRESS_EEPROM, a, &storage_data.pos_recent_log, 4);
        a = EEPROM_FULL_SIZE - 6;
        eeprom_write(ADDRESS_EEPROM, a, &storage_data.pos_recent_log, 4);
    }
    
    if(storage_data.manager_eeprom >= ADDRESS_LAST_LOG + SIZE_LOG) 
    {
        storage_data.pos_old_log = 0x0A; //checa se próximo supera a memória, adiantando o reset do log mais antigo, pois pode haver uma requisição e esse valor precisa estar atualizado, e assim não atualizando só quando for salvar
        storage_data.flag_is_inverted = !storage_data.flag_is_inverted;
    }
   
    return ESP_OK;
}

esp_err_t load_data(uint8_t *data, size_t size, uint32_t address)
{
    // address = storage_data.manager_eeprom - size_log;
    eeprom_read(ADDRESS_EEPROM, address, data, size);
    return ESP_OK;
}

//Usadas externamente

int storage_save_settings(void *settings)
{
    return 0;
}

int storage_load_settings(void *settings)
{
    return 0;
}

int check_num_logs(uint16_t* logs_disp, uint16_t required_logs)
{ 
    if (!storage_data.memory_full) 
    {
        uint16_t logs_saved = storage_data.pos_recent_log/10;
        if (storage_data.pos_recent_log == storage_data.manager_eeprom){
            *logs_disp = 0; 
            return ZERO_LOGS;
        }
        if (required_logs >= logs_saved){
            *logs_disp = logs_saved;
            return DEFAULT;
        }
        if (required_logs < logs_saved){
            *logs_disp = required_logs;
            return DEFAULT;
        }
            
    }else{
        //Dado que a capacidade de EEPROM é grande, não será enviado todos os logs
        *logs_disp = ADDRESS_LAST_LOG/SIZE_LOG;
        return PART_LOGS_FULL;
    }
    return -1;
}

int storage_init(void)
{   
    #ifdef RESET_M
            reset_memory();
            storage_managers_init();
            ESP_LOGI(TAG, "Dispositivo configurado com sucesso");
            return ESP_OK;
    #endif
    
    if (eeprom_info_init() == ESP_OK) 
        return ESP_OK;

    //Incia a checagem por brute force

    //Define storage_data.flag_is_inverted
    if( inversion_flag_update() != ESP_OK ){
        reset_memory();
        storage_managers_init();
        printf("Checagem de memoria falhou\nIniciando da posicao %d\n",ADDRESS_FIST_LOG);
        return ESP_OK;
    }
  
    uint32_t analyzed_log_address = ADDRESS_FIST_LOG,
             checkSum = 0, 
             checkSum_log = 0;
    uint8_t log[SIZE_LOG];
    printf("flag_is_inverted: %d\n",storage_data.flag_is_inverted);
    
    while (1)
    {   
        checkSum_log = 0;
        if((analyzed_log_address <= ADDRESS_LAST_LOG) && !storage_data.flag_is_inverted)
        {
            eeprom_read(ADDRESS_EEPROM, analyzed_log_address, log, SIZE_LOG);
            
            checkSum_log |= log[9]<<24 | log[8]<<16 | log[7]<<8 | log[6]; //montando o checkSum que foi desmenbrado em uint8
            checkSum = log[0] + ( log[4]<<24 | log[3]<<16 | log [2]<<8 | log[1]) + log[5]; //somando initial_flag_log + timestamp + datatype

            // checa se houve a inversão da flag, se sim, garante que aquele é o último log
            if (FLAG_IS_INVERT(log[0]))
            {
                if (checkSum == checkSum_log)
                    {
                        UPDATE_STRUCT_STORAGE(analyzed_log_address, analyzed_log_address - SIZE_LOG, analyzed_log_address, 1);
                        break;
                    }
                else
                    {   
                        // UPDATE_STRUCT_STORAGE(10, analyzed_log_address - SIZE_LOG, analyzed_log_address, 1);
                        reset_memory();
                        break;
                    }
            }

            if ( (!(FLAG_IS_DEFAULT(log[0]))) || (checkSum != checkSum_log))
            {   
                UPDATE_STRUCT_STORAGE(10, analyzed_log_address-SIZE_LOG, analyzed_log_address, 0);
                break;
            }
            
        }
        
        if((analyzed_log_address <= ADDRESS_LAST_LOG) &&  storage_data.flag_is_inverted)
        {
            eeprom_read(ADDRESS_EEPROM, analyzed_log_address, log, SIZE_LOG);
            checkSum_log |= log[9]<<24 | log[8]<<16 | log[7]<<8 | log[6]; //montando o checkSum que foi desmenbrado em uint8
            checkSum = log[0] + ( log[4]<<24 | log[3]<<16 | log [2]<<8 | log[1]) + log[5]; //somando initial_flag_log + timestamp + event/ataque
            
            // checa se houve a inversão da flag, se sim, garante que aquele é o último log
            if (FLAG_IS_DEFAULT(log[0]))
            {   
                if (checkSum == checkSum_log)
                    {
                        UPDATE_STRUCT_STORAGE(analyzed_log_address, analyzed_log_address - SIZE_LOG, analyzed_log_address, 1);
                        break;
                    }
                else
                    {   
                        // UPDATE_STRUCT_STORAGE(10, analyzed_log_address - SIZE_LOG, analyzed_log_address, 1);
                        reset_memory();
                        break;
                    }
            }

            if ((!(FLAG_IS_INVERT(log[0]))) || (checkSum != checkSum_log))
            {   
                UPDATE_STRUCT_STORAGE(10, analyzed_log_address-SIZE_LOG, analyzed_log_address, 0);
                break;
            }
        }
        
        //Toda a memória foi cheia com uma direcao da flag, então coincidentemente a ultima escrita foi no final da memoria
        //ou a variavel storage_data.flag_is_inverted corrompeu
        //adianta o próximo check
        if( analyzed_log_address > ADDRESS_LAST_LOG)
        {
          UPDATE_STRUCT_STORAGE(10, ADDRESS_LAST_LOG, 10, 1);
          storage_data.flag_is_inverted = !storage_data.flag_is_inverted;
          break;      
        }
        
    analyzed_log_address += SIZE_LOG;
    vTaskDelay(10/portTICK_PERIOD_MS);
    }
    save_eeprom_controller();
    storage_managers_init();
    return 0;
}

struct storage_device_t *storage_device_instance()
{
    if (storage_device.storage_init == 0)
    {
        storage_device = (struct storage_device_t){
            .storage_init = &storage_init,
            .save_settings = &storage_save_settings,
            .load_settings = &storage_load_settings,
            .save_data = &save_data,
            .load_data = &load_data,
            .load_memory = &load_memory,
            .check_num_logs = &check_num_logs,
            };
        //storage_init();
    }
    return &storage_device;
}