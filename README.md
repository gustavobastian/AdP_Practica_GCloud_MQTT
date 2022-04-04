# AdP 2022

## Conexión del ESP32 a Google Cloud IoT-Core

Configurar SSID y PASSWORD de la red Wi-Fi en el archivo smart_config.c

Configurar tu ID de dispositivo segun tu nombre, en el archivo mqtt_basico.h

El certificado incluido en el archivo pemhey.h fue generado en la consola de Google Cloud.
Se utilizo el siguiente comando:

openssl req -x509 -nodes -newkey rsa:2048 -keyout rsa_lz_private.pem -out rsa_lz_cert.pem -subj "/CN=unused" -days 36500 