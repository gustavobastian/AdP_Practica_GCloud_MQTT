#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "smart_config.h"
#include "nvs_flash.h"
#include "sntp_time.h"
#include "mqtt_basico.h"

extern bool time_sinc_ok;

void app_main(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );

    initialise_wifi();

    initialize_sntp();

    while (!time_sinc_ok) vTaskDelay(100 * 1);
    
    xTaskCreate(mqtt_app_main_task, "mqtt_app_task", 4096 * 8, NULL, 3, NULL);

    xTaskCreate(publicar_temperatura_task, "temp_pub_task", 4096 * 8, NULL, 3, NULL);
}

