/*
 * mqtt_basico.h
 *
 *  Created on: Mar 21, 2020
 *      Author: leopoldo
 */

#ifndef MAIN_MQTT_BASICO_H_
#define MAIN_MQTT_BASICO_H_

#include "stdlib.h"
#include "stdio.h"

// Datos de acceso a Google Cloud Platform
#define BROKER_URI				"mqtts://mqtt.googleapis.com:8883"
#define IOTCORE_USERNAME		"unused"
#define IOTCORE_PROJECTID		"adp-inicio-cloud"
#define IOTCORE_DEVICE_NUMBER	"100" // Completar el ID que te corresponda segun tu nombre:

// 101	Leopoldo Zimperz
// 102	Oscar Antonio
// 103	Paolo
// 104	RICARDO
// 105	Gustavo Adrian
// 106	Cynthia Mariel
// 107	Pablo Nicolás
// 108	Fernando
// 109	Juan Manuel
// 110	Ignacio Joaquin
// 111	Leonardo
// 112	Luciano Matías

#define IOTCORE_DEVICEID		"dispositivo-"IOTCORE_DEVICE_NUMBER
#define IOTCORE_REGION			"us-central1"
#define IOTCORE_REGISTRY		"adp-registry"
#define IOTCORE_CLIENTID		"projects/"IOTCORE_PROJECTID"/locations/"IOTCORE_REGION"/registries/"IOTCORE_REGISTRY"/devices/"IOTCORE_DEVICEID

#define IOTCORE_TOKEN_EXPIRATION_TIME_MINUTES		60 * 24

#define SENSOR_INTERVALO_MUESTREO_MINUTOS			1
#define SENSOR_CICLOS_MUESTREO_ENTRE_ENVIOS			5

#define ERROR_CODE_RESET	1
#define ERROR_CODE_JWT		2
#define ERROR_CODE_SNTP		4
#define ERROR_CODE_MQTT		8
#define ERROR_CODE_TIMEOUT  16
#define ERROR_CODE_WIFI  	32
#define ERROR_CODE_IP		64

void mqtt_app_main_task(void * parm);
void publicar_temperatura_task(void * parm);

extern int last_error_count;
extern int last_error_code;
extern unsigned int tph_on_time;
extern unsigned int last_on_time_seconds;
extern int last_sntp_response_time_seconds;

#endif /* MAIN_MQTT_BASICO_H_ */
