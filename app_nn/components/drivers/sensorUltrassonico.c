#include "sensorUltrassonico.h"

struct ultrasonic_driver_t ultrasonic_driver = {0}; // estrutura de metodos

esp_err_t config = ESP_FAIL;
uint8_t force_pass = 0;

/**
 * @brief instala estrutura de atributos
 * @return estrutura padrão de atributos
 *
 */
struct ultrasonic_driver_data_t ultrasonic_get_attributes(void)
{
    struct ultrasonic_driver_data_t ultrasonic_data = {
        // estrutura de atributos
        .u_frequency = ULTRASONIC_PWM_FREQUENCY,
        .u_timer = ULTRASONIC_TIMER,
        .u_timer_group = ULTRASONIC_TIMER_GROUP,
        .u_ledc_timer = ULTRASONIC_PWM_TIMER,
        .u_ledc_channel = ULTRASONIC_PWM_CHANNEL,
        .u_echo_pin = RECV_ECHO_PIN,
        // .u_threshold_pin = THRESHOLD_PIN,
        .u_pwm_pin = PWM_OUTPUT_PIN,
        .u_distance_threshold = 0
    };
    return ultrasonic_data;
}

QueueHandle_t xEcho_Queue = NULL;

// VARIAVEIS PRIVADAS ==============================================================
static const char *ULTRASONIC_TAG = "ULTRASONIC";
uint64_t ultrasonic_current_nod_time;

// PROTOTIPOS DE CALBACKS DE INTERRUPÇÃO ===========================================
static void IRAM_ATTR echo_isr_callback(void *isr_echo_pin);
static bool IRAM_ATTR ultrasonic_pwm_stop_isr_callback(void *args);


// ENCONTRO DO THRESHOLD DO ATM ===============================================
/**
 * @brief Coleta de dados para definição do threshold de distancia
 * @param echo_pin 
 * @return 
 *
 */
ultrasonic_status_t ultrasonic_init_threshold_distance(struct ultrasonic_driver_data_t *driver_data)
{
    struct ultrasonic_driver_t *ultra = ultrasonic_driver_instance();
    uint16_t total_distance = 0;
    uint8_t i = 1, distance, check = 0;
    for(; i <= 15; i++)
    {
        distance = ultra->get_distance(driver_data);
        if(distance == 0)
        {
            i--;
            check++;
            if(check>9){
                gpio_intr_disable((driver_data->u_echo_pin));
                ledc_timer_pause(LEDC_LOW_SPEED_MODE, driver_data->u_ledc_channel);
                timer_pause(driver_data->u_timer_group, driver_data->u_timer);
                return ULTRASONIC_FAIL;
            }  
            
            vTaskDelay(100/portTICK_PERIOD_MS);
            continue;
        }
    
        total_distance += distance;
        printf("a: %d\n",distance);
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    // gpio_intr_disable((driver_data->u_echo_pin));
    // ledc_timer_pause(LEDC_LOW_SPEED_MODE, driver_data->u_ledc_channel);
    // timer_pause(driver_data->u_timer_group, driver_data->u_timer);
    driver_data->u_distance_threshold = total_distance/i;
    
    if (driver_data->u_distance_threshold < MINIMAL_DISTANCE)
        return ULTRASONIC_FAIL;

    return ULTRASONIC_SUCCESS;
}


// TASK ULTRASSONICO
void ultrasonic_task_get_distance(void*pvParameters)
{   
    struct ultrasonic_driver_data_t ultra_attributes = ultrasonic_get_attributes();
    struct ultrasonic_driver_t *ultra = ultrasonic_driver_instance();
    if (ultra->init_data(&ultra_attributes) == ULTRASONIC_SUCCESS)
        config = ESP_OK;
    else
        force_pass = 1;

    // printf("ultra_attributes->u_distance_threshold: %d\n",ultra_attributes.u_distance_threshold);
    #ifdef SAVE_EEPROM
        struct storage_device_t *storage_eeprom = storage_device_instance();
        extern struct storage_data_t storage_data;
    #endif

    uint32_t log_data[4],
             total, 
             checksum;
            
    time_t tt;

    uint8_t distance, 
            in_limit = true,
            threshold = ultra_attributes.u_distance_threshold,
            upper_threshold = threshold + TRESHOLD_RANGE,
            lower_threshold,
            id_flag_log,
            ite = 8,
            erro;

    if ( (threshold-TRESHOLD_RANGE) > MINIMAL_DISTANCE)
        lower_threshold = threshold - TRESHOLD_RANGE;
    else
        lower_threshold = MINIMAL_DISTANCE;
        

    ESP_LOGI(ULTRASONIC_TAG, "Iniciando a Task"); 
    for(;;)
    {  
        total = 0;
        for(uint8_t i = 0; i<ite; i++){
            distance = ultra->get_distance(&ultra_attributes);
            total += distance;
            vTaskDelay(50/portTICK_PERIOD_MS);
        }

        erro = total%ite; //Quantos valores ficaram acima da média inteira
        if(erro>=3)
            distance = total/ite;
        else
            distance = total/ite + 1;

        #ifdef SHOW_ULTRA_RESULTS
            printf("cm: %d\tticks: %d\ttempo: %d\n", distance, xTaskGetTickCount(), pdTICKS_TO_MS(xTaskGetTickCount()));
        #endif

        if(in_limit){
            if(distance < lower_threshold || distance > upper_threshold){
                printf("saiu\n");
                #ifdef SAVE_EEPROM
                    tt = time(NULL);
                    if (tt<1704067200) //1 jan 2024 00:00 GMT
                        tt = 0;

                    if (storage_data.flag_is_inverted)  id_flag_log = flag_inverter_ultra;
                    if (!storage_data.flag_is_inverted) id_flag_log = flag_default_ultra;

                    checksum = (uint32_t) tt + (uint32_t) id_flag_log + (uint32_t) distance;
                    
                    log_data[0] = id_flag_log;
                    log_data[1] = tt;
                    log_data[2] = distance;
                    log_data[3] = checksum;
        
                    xQueueSend(storage_data.save_logs_queue, log_data, 0);
                    vTaskDelay(550/portTICK_PERIOD_MS);
                #endif

                in_limit = false;
            }
        }

        if(!in_limit){
            if(distance >= lower_threshold && distance <= upper_threshold){
                printf("Entrou\n");
                #ifdef SAVE_EEPROM
                    tt = time(NULL);
                    if (tt<1704067200) //1 jan 2024 00:00 GMT
                        tt = 0;

                    if (storage_data.flag_is_inverted)  id_flag_log = flag_inverter_ultra;
                    if (!storage_data.flag_is_inverted) id_flag_log = flag_default_ultra;

                    checksum = (uint32_t) tt + (uint32_t) id_flag_log + (uint32_t) distance;
                    
                    log_data[0] = id_flag_log;
                    log_data[1] = tt;
                    log_data[2] = distance;
                    log_data[3] = checksum;
                    
                    xQueueSend(storage_data.save_logs_queue, log_data, 0);
                    vTaskDelay(550/portTICK_PERIOD_MS);
                #endif

                in_limit = true;
            }
        }
        
        vTaskDelay(50 / portTICK_PERIOD_MS);
        
    }
}

// DEFINIÇÕES DE FUNÇÕES DE INICIALIZAÇÃO ==========================================

/**
 * @brief inicialização do pino de echo
 * @param echo_pin pino de echo
 * @return ultrasonic_status_t
 *
 */
ultrasonic_status_t ultrasonic_init_echo(uint8_t *echo_pin)
{
    esp_err_t ultrasonic_err;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1 << (*echo_pin)),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_POSEDGE,
    };

    ultrasonic_err = gpio_config(&io_conf);
    if (ultrasonic_err != ESP_OK)
    {
        ESP_LOGE(ULTRASONIC_TAG, "erro ao configurar echo gpio\terro: %d", ultrasonic_err);
        return ULTRASONIC_FAIL;
    }

    // gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    gpio_isr_handler_add((*echo_pin), echo_isr_callback, (void *)echo_pin);
    gpio_intr_disable((*echo_pin));

    return ULTRASONIC_SUCCESS;
}

/**
 * @brief inicialização do pino de threshold
 * @param threshold_pin pino de threshold
 * @return ultrasonic_status_t
 *
 */
ultrasonic_status_t ultrasonic_init_threshold(uint8_t threshold_pin)
{
    esp_err_t ultrasonic_err;

    gpio_config_t io_confo = {
        .pin_bit_mask = (1 << threshold_pin),
        .mode = GPIO_MODE_OUTPUT,
    };

    ultrasonic_err = gpio_config(&io_confo);
    if (ultrasonic_err != ESP_OK)
    {
        ESP_LOGE(ULTRASONIC_TAG, "erro ao configurar threshold gpio\terro: %d", ultrasonic_err);
        return ULTRASONIC_FAIL;
    }

    return ULTRASONIC_SUCCESS;
}

/**
 * @brief inicialização de timer
 * @param group grupo de timers do ESP32
 * @param timer timer a ser utilizado
 * @return ultrasonic_status_t
 *
 */
ultrasonic_status_t ultrasonic_timer_init(struct ultrasonic_driver_data_t *driver_data)
{
    esp_err_t ultrasonic_err;

    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = TIMER_AUTORELOAD_EN,
    };
    ultrasonic_err = timer_init(driver_data->u_timer_group, driver_data->u_timer, &config);

    if (ultrasonic_err != ESP_OK)
    {
        ESP_LOGE(ULTRASONIC_TAG, "erro ao configurar timer\terro: %d", ultrasonic_err);
        return ULTRASONIC_FAIL;
    }

    timer_set_counter_value(driver_data->u_timer_group, driver_data->u_timer, 0);             // reincia o contador do timer
    timer_set_alarm_value(driver_data->u_timer_group, driver_data->u_timer, 1 * TIMER_SCALE); // configura a escala de tempo
    timer_enable_intr(driver_data->u_timer_group, driver_data->u_timer);
    timer_isr_callback_add(driver_data->u_timer_group, driver_data->u_timer, ultrasonic_pwm_stop_isr_callback, (void *)driver_data, 0);

    ultrasonic_err = timer_start(driver_data->u_timer_group, driver_data->u_timer);
    if (ultrasonic_err != ESP_OK)
    {
        ESP_LOGE(ULTRASONIC_TAG, "erro ao iniciar timer\terro: %d", ultrasonic_err);
        return ULTRASONIC_FAIL;
    }

    return ULTRASONIC_SUCCESS;
}

/**
 * @brief inicializa gerador PWM
 * @param pwm_pin pino de saida PWM
 * @param freq frequencia do sinal PWM
 * @param ledc_timer timer ledc para sina PWM
 * @param ledc_channel canal ledc para PWM
 * @return ultrasonic_status_t
 *
 */
ultrasonic_status_t ultrasonic_pwm_init(struct ultrasonic_driver_data_t *driver_data)
{
    esp_err_t ultrasonic_err;

    ledc_timer_config_t U_ledc_timer = {
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = driver_data->u_frequency,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = driver_data->u_ledc_timer,
        .clk_cfg = LEDC_AUTO_CLK};

    ultrasonic_err = ledc_timer_config(&U_ledc_timer);

    if (ultrasonic_err != ESP_OK)
    {
        ESP_LOGE(ULTRASONIC_TAG, "erro ao configurar ledc timer\terro: %d", ultrasonic_err);
        return ULTRASONIC_FAIL;
    }

    // configuração do canal PWM
    ledc_channel_config_t U_ledc_channel = {
        .channel = driver_data->u_ledc_channel,
        .duty = 511, // 50% de duty cycle
        .gpio_num = driver_data->u_pwm_pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = driver_data->u_ledc_timer};
    ledc_channel_config(&U_ledc_channel);

    if (ultrasonic_err != ESP_OK)
    {
        ESP_LOGE(ULTRASONIC_TAG, "erro ao configurar canal ledc\terro: %d", ultrasonic_err);
        return ULTRASONIC_FAIL;
    }

    return ULTRASONIC_SUCCESS;
}

ultrasonic_status_t ultrasonic_init()
{
    xTaskCreate(ultrasonic_task_get_distance,"ultrasonic_task_get_distance", 2048, NULL,10,NULL);

    while(config != ESP_OK && !force_pass)
        vTaskDelay(10/portTICK_PERIOD_MS);

    if(config == ESP_OK)
        return ULTRASONIC_SUCCESS;
    else
        return ULTRASONIC_FAIL;
} 

/**
 * @brief chama funções de inicilazação do sensor ultrasonico
 * @param driver_data endereço para estrutua de atriutos dosensor ultrasonico
 * @return ultrasonic_status_t
 *
 */
ultrasonic_status_t ultrasonic_init_data(struct ultrasonic_driver_data_t *driver_data)
{
   
    ultrasonic_status_t ultrasonic_err;
    
    xEcho_Queue = xQueueCreate(3, sizeof(bool));
   
    ultrasonic_err = ultrasonic_init_echo(&(driver_data->u_echo_pin));
    
    if (ultrasonic_err != ESP_OK)
    {
        ESP_LOGE(ULTRASONIC_TAG, "erro ao iniciar echo\terro: %d", ultrasonic_err);
        return ULTRASONIC_FAIL;
    }
    
    // ultrasonic_err = ultrasonic_init_threshold(driver_data->u_threshold_pin);
    // if (ultrasonic_err != ESP_OK)
    // {
    //     ESP_LOGE(ULTRASONIC_TAG, "erro ao iniciar threshold\terro: %d", ultrasonic_err);
    //     return ULTRASONIC_FAIL;
    // }

    ultrasonic_err = ultrasonic_pwm_init(driver_data);
    if (ultrasonic_err != ESP_OK)
    {
        ESP_LOGE(ULTRASONIC_TAG, "erro ao iniciar pwm\terro: %d", ultrasonic_err);
        return ULTRASONIC_FAIL;
    }

    ultrasonic_err = ultrasonic_timer_init(driver_data);
    if (ultrasonic_err != ESP_OK)
    {
        ESP_LOGE(ULTRASONIC_TAG, "erro ao iniciar timer\terro: %d", ultrasonic_err);
        return ULTRASONIC_FAIL;
    }

      
    ESP_LOGI(ULTRASONIC_TAG, "Ultrassonico configurado com sucesso"); 
    #ifdef TRESHOLD_ULTRA
        driver_data->u_distance_threshold = TRESHOLD_ULTRA;
    #else
        ultrasonic_err = ultrasonic_init_threshold_distance(driver_data);
        if (ultrasonic_err != ESP_OK)
        {
            ESP_LOGE(ULTRASONIC_TAG, "erro ao iniciar o threshold de distancia %d | %d cm", ultrasonic_err, driver_data->u_distance_threshold);
            return ULTRASONIC_FAIL;
        }
        else{
            ESP_LOGI(ULTRASONIC_TAG, "Threshold do ultrassonico em %d cm", driver_data->u_distance_threshold); 
        }
    #endif
    
    
    return ULTRASONIC_SUCCESS;
}

// DEFINIÇÕES DE FUNÇÕES DE OPERAÇÃO ===============================================
/**
 * @brief inicializa transmissão de sinal PWM de 8 pulsos
 * @param group grupo de timer do ESP32
 * @param timer timer a ser utilizado
 * @param ledc_channel canal ledc para PWM
 * @return ultrasonic_status_t
 *
 * @note os 8 pulsos gerados coma inicialização de um sinal PWM que é interrompido com a chamada de uma
 *       função de calback de estouro de timer. O timer em questão conta o periodo dos 8 pulsos.
 */
ultrasonic_status_t ultrasonic_nod(timer_group_t group, timer_idx_t timer, ledc_channel_t ledc_channel)
{
    esp_err_t ultrasonic_err;

    // iniciar timer
    timer_set_counter_value(group, timer, 0);
    ultrasonic_err = timer_start(group, timer);
    if (ultrasonic_err != ESP_OK)
    {
        ESP_LOGE(ULTRASONIC_TAG, "erro ao iniciar timer para o sinal de aceno\terro: %d", ultrasonic_err);
        return ULTRASONIC_FAIL;
    }

    // reseta sinal PWM
    ledc_timer_rst(LEDC_LOW_SPEED_MODE, ledc_channel);
    ultrasonic_err = ledc_timer_resume(LEDC_LOW_SPEED_MODE, ledc_channel);

    if (ultrasonic_err != ESP_OK)
    {
        ESP_LOGE(ULTRASONIC_TAG, "erro ao iniciar timer ledc para o sinal de aceno\terro: %d", ultrasonic_err);
        return ULTRASONIC_FAIL;
    }

    return ULTRASONIC_SUCCESS;
}

/**
 * @brief função de calback para parada de sinal PWM
 * @param group grupo de timer do ESP32
 * @param timer timer a ser utilizado
 * @param ledc_channel canal ledc para PWM
 * @return ultrasonic_status_t
 *
 */
ultrasonic_status_t ultrasonic_stop_nod_callback(timer_group_t group, timer_idx_t timer, ledc_channel_t ledc_channel)
{
    esp_err_t ultrasonic_err;

    // pausa sinal PWM
    ultrasonic_err = ledc_timer_pause(LEDC_LOW_SPEED_MODE, ledc_channel);
    ultrasonic_current_nod_time = esp_timer_get_time();

    timer_pause(group, timer);
    if (ultrasonic_err != ESP_OK)
    {
        ESP_LOGE(ULTRASONIC_TAG, "erro ao parar timer ledc para o sinal de aceno\terro: %d", ultrasonic_err);
        return ULTRASONIC_FAIL;
    }

    return ULTRASONIC_SUCCESS;
}

/**
 * @brief calcula a distância até um objeto de acordo com o tempo decorrido desde a finalização do sinal PWM
 *        até a recepção do sinal no pino de echo.
 * @param driver_data endereço para estrutua de atriutos dosensor ultrasonico
 *
 */
uint32_t ultrasonic_get_distance(struct ultrasonic_driver_data_t *driver_data)
{
    uint32_t distance = 0;
    uint8_t echo_flag = 0;

    ultrasonic_nod(driver_data->u_timer_group, driver_data->u_timer, driver_data->u_ledc_channel);

    // gera timeout para receber estado da flag
    xQueueReceive(xEcho_Queue, &echo_flag, ULTRASONIC_TIMEOUT / portTICK_PERIOD_MS);
    echo_flag = 0;

    gpio_intr_enable(driver_data->u_echo_pin);
    xQueueReceive(xEcho_Queue, &echo_flag, ULTRASONIC_TIMEOUT / portTICK_PERIOD_MS);

    if (echo_flag)
        distance = ultrasonic_current_nod_time / CM_CONVERTION_FACTOR / 2;

    return distance;
}

// FUNÇÕES DE CALLBACK PARA INTERRUPÇÃO =============================================
// Interrupcao por Timer
/**
 * @brief interrompe o sinal PWM
 * @param args endereço da estrutura de atributos 
 * @return bool
 *
 * @note contem envio de flag para gerar timeout na função de get_distance
 *
 */
static bool IRAM_ATTR ultrasonic_pwm_stop_isr_callback(void *args)
{
    struct ultrasonic_driver_data_t *isr_driver_data = (struct ultrasonic_driver_data_t *)args;
    BaseType_t high_task_awoken = pdFALSE;
    bool timeout_flag = 0; // valor escrito para gerar o timeout

    ultrasonic_stop_nod_callback(isr_driver_data->u_timer_group, isr_driver_data->u_timer, isr_driver_data->u_ledc_channel);
    xQueueSendFromISR(xEcho_Queue, &timeout_flag, NULL);

    return high_task_awoken == pdTRUE;
}

// Interrupcao externa por GPIO
/**
 * @brief desabilita recepção de sinal pelo pino de echo.
 *        atualiza tempo decorrido entre o envio do sinal PWM e recepção de echo.
 *        sinaliza detecção de sinal de echo
 * @param args parametros para função de calback - NÃO UTILIZADO
 * @note configurar pino de echo no arquivo sensorUltrassonico.h
 *
 */
static void IRAM_ATTR echo_isr_callback(void *isr_echo_pin)
{
    bool echo_trigg = 1;
    gpio_intr_disable(*((uint8_t *) isr_echo_pin));                                   // desabilita interrupção para evitar chamadas desnecessárias do callback

    ultrasonic_current_nod_time = esp_timer_get_time() - ultrasonic_current_nod_time; // atualiza tempo decorrido para o retorno do sinal ultrassonico
    xQueueSendFromISR(xEcho_Queue, &echo_trigg, NULL);
}

/**
 *  @brief externalização de funções
 *  @return endereço para (struct ultrasonic_driver_t)
 *
 */
struct ultrasonic_driver_t *ultrasonic_driver_instance()
{
    if (ultrasonic_driver.init == NULL)
    {
        ultrasonic_driver = (struct ultrasonic_driver_t){
            .init = &ultrasonic_init,
            .init_data = &ultrasonic_init_data,
            .get_distance = &ultrasonic_get_distance,
        };
    }
    return &ultrasonic_driver;
}
