#include "acc.h"

struct acc_driver_data_t acc_driver_data;
struct acc_driver_t acc_driver;

float  acc_x_data[50];
float  acc_y_data[50];
float  acc_z_data[50];
uint8_t autoteste_flag = 0;

struct lis3dh_driver_t *acc_get_methods_lis3dh()
{
    return acc_driver_data.lis3dh_driver;
}

esp_err_t acc_autoteste(void)
{
    autoteste_flag = 1;
    printf("Autoteste ACC \n");
    for (size_t i = 0; i < 50; i++)
    {
        if ((acc_x_data[i] >= 3.0) & (acc_x_data[i] <= -3.0))
        {
            autoteste_flag = 0;
            return ESP_FAIL;
        }
        if ((acc_y_data[i] >= 3.0) & (acc_y_data[i] <= -3.0))
        {
            autoteste_flag = 0;
            return ESP_FAIL;
        }
        if ((acc_z_data[i] >= 3.0) & (acc_z_data[i] <= -3.0))
        {
            autoteste_flag = 0;
            return ESP_FAIL;
        }
        
    }
    
    autoteste_flag = 0;
    return ESP_OK;
}

float formatData(int16_t pData)
{
    return ((float)pData / 1000.0);
}

int acc_handler()
{
    struct lis3dh_driver_t *acc_lis3dh_methods = acc_get_methods_lis3dh();
    struct sensors_t *sensors_methods = sensors_instance();

    uint8_t flags;
    uint8_t num = 0;
    uint8_t buf = 0;
    
    int16_t data_acc[3];
    float aceleration_2g[3];

    /* Check if FIFO level over threshold */
    acc_lis3dh_methods->lis3dh_status_fifo(&buf);
    flags = buf >> 7;
    num = buf & (0x1F);
    if (flags) {
      /* Read number of sample in FIFO */
      while (num-- > 0) {
        /* Read XL samples */
        acc_lis3dh_methods->lis3dh_read_data(data_acc);
        
        aceleration_2g[0] = formatData(data_acc[0]);
        aceleration_2g[1] = formatData(data_acc[1]);
        aceleration_2g[2] = formatData(data_acc[2]);
        
        if(num == 31)   break;        

        if ((acc_driver_data.count_int_first < 50))
        {
            acc_x_data[acc_driver_data.count_int_first] = aceleration_2g[0];
            acc_y_data[acc_driver_data.count_int_first] = aceleration_2g[1];
            acc_z_data[acc_driver_data.count_int_first] = aceleration_2g[2];
            if(acc_driver_data.count_int_first%10 == 0)
            {
            // printf("acc_x_data[acc_driver_data.count_int_first]: %f\n", acc_x_data[acc_driver_data.count_int_first]);
            // printf("acc_y_data[acc_driver_data.count_int_first]: %f\n", acc_y_data[acc_driver_data.count_int_first]);
            // printf("acc_z_data[acc_driver_data.count_int_first]: %f\n", acc_z_data[acc_driver_data.count_int_first]);
            }
            acc_driver_data.count_int_first++;
        }
        else if ((acc_driver_data.count_int_first >= 50))
        {
            if(!autoteste_flag) sensors_methods->sensors_refresh_acc(&acc_x_data, &acc_y_data, &acc_z_data);
            acc_driver_data.count_int_first = 0;
        }
      }
    }
    return 0;
}

/**
 * @brief tratamento de interrupcao esp32
 * 
 * @param arg 
 */
static void IRAM_ATTR acc_esp_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(acc_driver_data.acc_irq_evt_queue, &gpio_num, NULL);
}

/**
 * @brief task de notificacao da interrupcao do accel
 * 
 * @param arg 
 */
static void acc_task_notify_irq(void *arg)
{
    uint32_t io_num;
    acc_handler();
    for (;;)
    {
        if (xQueueReceive(acc_driver_data.acc_irq_evt_queue, &io_num, 10/* portMAX_DELAY */))
        {
            acc_handler();
        }
    }
}

/**
 * @brief cadastro de gpios para interrupcao
 * 
 * @param io 
 * @param acc_irq_handle 
 * @return int 
 */
int acc_register_irq(void)
{
    // código para cadastrar a irq para as GPIOs desejadas
    gpio_config_t io_conf = {0};
    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    // bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_14);
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    //gpio_set_pull_mode(GPIO_NUM_14,GPIO_PULLUP_ONLY);

    // create a queue to handle gpio event from isr
    acc_driver_data.acc_irq_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // start gpio task
    xTaskCreate(acc_task_notify_irq, "acc_task_notify_irq", 2048, NULL, 10, acc_driver_data.xHandle_acc);

    // install gpio isr service
    esp_err_t err = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);
    // hook isr handler for specific gpio pin
    if(err != ESP_OK){
        ESP_LOGE("ACC", "Interrupção não instalada");
        return ESP_FAIL;
    }

    err = gpio_isr_handler_add(GPIO_NUM_14, acc_esp_isr_handler, (void *)GPIO_NUM_14);

    if(err != ESP_OK){
        ESP_LOGE("ACC", "Interrupção mal configurada");
        return ESP_FAIL;
    }


    ESP_LOGI("ACC", "interrupção configurada com sucesso");
    return 0;
}

/**
 * @brief inicializacao do acelerometro
 * 
 * @return esp_err_t 
 */
esp_err_t acc_init(void)
{
    acc_driver_data.lis3dh_driver = lis3dh_driver_instance();
    struct lis3dh_driver_t *acc_lis3dh_methods = acc_get_methods_lis3dh();
    acc_driver_data.count_int_first = 0;
    esp_err_t err_acc = acc_lis3dh_methods->lis3dh_init();
    
    if (err_acc != ESP_OK)
    {
        ESP_LOGE("ACC", "ACC NÃO INICIOU");
        return ESP_FAIL;
    }

    ESP_LOGI("ACC", "ACC INICIOU");

    acc_register_irq();

    return ESP_OK;    
}

/**
 * @brief Leitura de dados do acelerometro
 * 
 * @return esp_err_t 
 */
esp_err_t acc_read(float *axis_x, float *axis_y, float *axis_z)
{
    struct lis3dh_driver_t *acc_lis3dh_methods = acc_get_methods_lis3dh();
    int16_t data_acc[3];
    acc_lis3dh_methods->lis3dh_read_data(data_acc);
    
    return ESP_OK;
}


struct acc_driver_t *acc_driver_instance()
{
    if (acc_driver.acc_init == 0)
    {
        acc_driver = (struct acc_driver_t)
        {
            .acc_init = &acc_init,
            .acc_read = &acc_read,
            .acc_autoteste = &acc_autoteste,
        };
    }
    return &acc_driver;
}
