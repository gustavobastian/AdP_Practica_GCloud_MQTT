/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "pemkey.h"

#include "mqtt_basico.h"
#include <string.h>
#include "esp_system.h"
#include "esp_event.h"

#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "esp_wifi.h"

#include "cJSON.h"

#include "sntp_time.h"

#include "jwt_token_gcp.h"

#include "esp_random.h"
#include "esp_sleep.h"

static const char *TAG = "MQTT MODULE: ";

esp_mqtt_client_handle_t cliente = NULL;

int RTC_DATA_ATTR last_error_count = 0;
int RTC_DATA_ATTR last_error_code = 0;
static unsigned char RTC_DATA_ATTR forzar_espera_sntp = 0;
unsigned int RTC_DATA_ATTR last_on_time_seconds = 0;
int sntp_response_time_seconds = 0;
unsigned int tph_on_time = 0;
time_t wake_up_timestamp = 0;


char MQTT_suffix[200];
char MQTT_topic[250];
char MQTT_payload[200];
size_t MQTT_payload_length = 200;
char MQTT_valor[100];
size_t MQTT_values_count;

esp_mqtt_client_handle_t mqtt_client_handle = NULL;
esp_mqtt_client_config_t mqtt_client_config = { };
char* GCP_JWT = NULL;
bool mqtt_client_connected = false;
bool mqtt_disconnected_event_flag = false;


static bool mqtt_client_configure(void);

int T = 0, P = 0, H = 0; // las declaro global para probar rapidamente
uint8_t id_sensor_recibido;


static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    cliente = event->client;
    switch (event->event_id) {
            case MQTT_EVENT_CONNECTED:
                ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

                // Suscribirse a tema 'config' de Google Cloud IoT
                esp_mqtt_client_subscribe(event->client, "/devices/"IOTCORE_DEVICEID"/config", 0);

                // Suscribirse a tema 'commands' de Google Cloud IoT
                esp_mqtt_client_subscribe(event->client, "/devices/"IOTCORE_DEVICEID"/commands/#", 0);
                mqtt_client_connected = true;
                break;

            case MQTT_EVENT_DISCONNECTED:
                ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
                //ESP_LOGI(TAG, "Resetting MQTT Config");
                //mqtt_client_reset_config();
                mqtt_client_connected = false;
                mqtt_disconnected_event_flag = true;
                break;

            case MQTT_EVENT_SUBSCRIBED:
                ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);		
                break;

            case MQTT_EVENT_UNSUBSCRIBED:
                ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
                break;

            case MQTT_EVENT_PUBLISHED:
                ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
                last_error_count = 0;
                last_error_code = 0;
                last_on_time_seconds = 0;
                forzar_espera_sntp = 0;
                sntp_response_time_seconds = 0;
                break;

            case MQTT_EVENT_DATA:
                ESP_LOGI(TAG, "MQTT_EVENT_DATA: ");
                printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
                printf("DATA=%.*s\r\n", event->data_len, event->data);
                break;

            case MQTT_EVENT_ERROR:
                ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
                // Si se presento error en la conexion MQTT, tal vez esta mal el horario del token.
                // Forzar espera de sincronizacion en el proximo reinicio.
                forzar_espera_sntp = 1;
                last_error_count ++;
                last_error_code |= ERROR_CODE_MQTT;
                break;

            default:
                ESP_LOGI(TAG, "Other event id:%d", event->event_id);
                break;
        }
        return ESP_OK;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}


void mqtt_app_main_task(void * parm)
{
    ESP_LOGI(TAG, "Ingresa a mqtt_app_main_task()");

    mqtt_client_configure();

    esp_mqtt_client_handle_t mqtt_client_handle = esp_mqtt_client_init(&mqtt_client_config);
    esp_mqtt_client_register_event(mqtt_client_handle, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client_handle);

    ESP_LOGI(TAG, "Arrancando MQTT client... ");
    esp_mqtt_client_start(mqtt_client_handle);


	while (1) {

		while(!mqtt_disconnected_event_flag) vTaskDelay(1000 / portTICK_PERIOD_MS);

		ESP_LOGI(TAG, "Actualizando configuracion Cliente MQTT... ");
		if (mqtt_client_configure()) {
			ESP_LOGI(TAG, "Seteando configuracion Cliente MQTT... ");
			esp_mqtt_set_config(mqtt_client_handle, &mqtt_client_config);
			ESP_LOGI(TAG, "Reseteando flag evento disconnected. ");		
			mqtt_disconnected_event_flag = false;			
		}

	}

	vTaskDelete(NULL);

}


static bool mqtt_client_configure(void) {

	ESP_LOGI(TAG, "Generando JWT Token... ");
    if (GCP_JWT != NULL) free(GCP_JWT);
	
	GCP_JWT = createGCPJWT(IOTCORE_PROJECTID, GCP_PEM_KEY, strlen(GCP_PEM_KEY), IOTCORE_TOKEN_EXPIRATION_TIME_MINUTES);

    if (GCP_JWT == 0) {
        last_error_count ++;
        last_error_code |= ERROR_CODE_JWT;
		ESP_LOGI(TAG, "Error al generar JWT Token... ");
		return false;
    }

    mqtt_client_config.uri = BROKER_URI;
    mqtt_client_config.username = IOTCORE_USERNAME;
    mqtt_client_config.password = GCP_JWT; //IOORE_TOKEN;
	//mqtt_client_config.disable_auto_reconnect = true;

    //printf("pass JWT: %s-FIN", GCP_JWT);
    mqtt_client_config.client_id = IOTCORE_CLIENTID;

	ESP_LOGI(TAG, "JWT Token generado... ");
	return true;
}

void publicar_temperatura_task(void * parm)
{
	ESP_LOGI(TAG, "Ingresa a publicar_temperatura_task()");

	uint16_t intervalo_publicacion_segundos = 30;

	char bufferJson[300];
	char bufferTopic[350];
	int msg_id;
	char buffer_temp_txt[15];
	char buffer_rssi_txt[15];
	
	float temp = 24;
    int8_t rssi = 0;
	wifi_ap_record_t ap_info;

    uint32_t random_number;


	while(1)
	{
        if (mqtt_client_connected) {
            // INICIO CICLO DE LECTURAS y publicaciones.
            vTaskDelay(intervalo_publicacion_segundos * 1000 / portTICK_PERIOD_MS);

            // Tomo la muestra del supuesto sensor, en este ejemplo solo genero un random
            ESP_LOGI(TAG, "Tomando muestra... ");
            random_number = esp_random();
            if (random_number >  2147483648)
                temp += 0.3;
                else 
                    temp -= 0.3;
            
            if (temp > 40) temp = 39.5;
            if (temp < 5) temp = 5.5;
            
                
            // Consulto al modulo el nivel de señal que esta recibiendo.
            esp_wifi_sta_get_ap_info(&ap_info);
            rssi = ap_info.rssi;
            ESP_LOGI(TAG, "RSSI: %d ", ap_info.rssi);

            // Convertir a texto los valores.
            snprintf(buffer_temp_txt, sizeof(buffer_temp_txt), "%.2f", temp);
            snprintf(buffer_rssi_txt, sizeof(buffer_rssi_txt), "%d", rssi);
            ESP_LOGI("Muestra: ", "T %s - RSSI %s", buffer_temp_txt, buffer_rssi_txt);

                bufferJson[0] = 0;

                strcat(bufferJson, "{ \"id_dispositivo\": ");
                strcat(bufferJson, IOTCORE_DEVICE_NUMBER);
                strcat(bufferJson, ", \"temperatura\": ");
                strcat(bufferJson, buffer_temp_txt);
                strcat(bufferJson, ", \"rssi\": ");
                strcat(bufferJson, buffer_rssi_txt);
                strcat(bufferJson, " }");

                ESP_LOGI("JSON enviado:", " %s ", bufferJson);

                bufferTopic[0] = 0;
                strcat(bufferTopic, "/devices/");
                strcat(bufferTopic, IOTCORE_DEVICEID);
                strcat(bufferTopic, "/events/temperatura");

                msg_id = esp_mqtt_client_publish(cliente, bufferTopic, bufferJson, 0, 1, 0);

                ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            // FIN CICLO LECTURA Y ENVIO
        }
	}
    vTaskDelete(NULL);
}