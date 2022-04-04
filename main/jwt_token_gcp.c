/*
 * jwt_token_gcp.c
 *
 *  Created on: Jul 16, 2020
 *      Author: leopoldo
 */

#include "jwt_token_gcp.h"


#include <stdio.h>
#include <string.h>
#include <mbedtls/pk.h>
#include <mbedtls/error.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_event.h>
//#include <esp_event_loop.h>
#include <esp_event.h>
#include <nvs_flash.h>
//#include <tcpip_adapter.h>
#include "esp_netif.h"
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_log.h"



//#include "passwords.h"
#include "base64url.h"

// This is an "xxd" file of the PEM of the private key.
// #include "device1_private_pem.h"



/**
 * Return a string representation of an mbedtls error code
 */
static char* mbedtlsError(int errnum) {
    static char buffer[200];
    mbedtls_strerror(errnum, buffer, sizeof(buffer));
    return buffer;
} // mbedtlsError


/**
 * Create a JWT token for GCP.
 * For full details, perform a Google search on JWT.  However, in summary, we build two strings.  One that represents the
 * header and one that represents the payload.  Both are JSON and are as described in the GCP and JWT documentation.  Next
 * we base64url encode both strings.  Note that is distinct from normal/simple base64 encoding.  Once we have a string for
 * the base64url encoding of both header and payload, we concatenate both strings together separated by a ".".   This resulting
 * string is then signed using RSASSA which basically produces an SHA256 message digest that is then signed.  The resulting
 * binary is then itself converted into base64url and concatenated with the previously built base64url combined header and
 * payload and that is our resulting JWT token.
 * @param projectId The GCP project.
 * @param privateKey The PEM or DER of the private key.
 * @param privateKeySize The size in bytes of the private key.
 * @returns A JWT token for transmission to GCP.
 */
char* createGCPJWT(char* projectId, unsigned char* privateKey, size_t privateKeySize, uint16_t expiration_minutes) {

//	ESP_LOGI("CreateJWT", "Creando JWT inicio");

	privateKeySize ++; //Le sumo un caracter al largo, porque para armar el JWT necesita incluir el caracter de terminacion '\0'.

    char base64Header[100];
    const char header[] = "{\"typ\": \"JWT\",\"alg\": \"RS256\"}";
    base64url_encode(
        (unsigned char *)header,   // Data to encode.
        strlen(header),            // Length of data to encode.
        base64Header);             // Base64 encoded data.

	ESP_LOGI("CreateJWT", "header: %s ", header);

    time_t now;
    time(&now);
    printf("Tiempo ahora time() %ld ", now);
    uint32_t iat = now;              // Set the time now.
    uint32_t exp = iat + 60 * expiration_minutes;      // Set the expiry time.

    char payload[100];
    sprintf(payload, "{\"aud\": \"%s\", \"iat\": %d, \"exp\": %d}", projectId, iat, exp);

	ESP_LOGI("CreateJWT", "payload: %s ", payload);

    char base64Payload[100];
    base64url_encode(
        (unsigned char *)payload,  // Data to encode.
        strlen(payload),           // Length of data to encode.
        base64Payload);            // Base64 encoded data.

    uint8_t headerAndPayload[800];
    sprintf((char*)headerAndPayload, "%s.%s", base64Header, base64Payload);

//	ESP_LOGI("Token ", "header and payload jwt sin firmar: %s ", headerAndPayload);

    // At this point we have created the header and payload parts, converted both to base64 and concatenated them
    // together as a single string.  Now we need to sign them using RSASSA

    mbedtls_pk_context pk_context;
    mbedtls_pk_init(&pk_context);
    int rc = mbedtls_pk_parse_key(&pk_context, privateKey, privateKeySize, NULL, 0);
    if (rc != 0) {
        printf("Failed to mbedtls_pk_parse_key: %d (-0x%x): %s\n", rc, -rc, mbedtlsError(rc));
        return 0;
    }

    uint8_t oBuf[5000];

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    const char* pers="MyEntropy";

    mbedtls_ctr_drbg_seed(
        &ctr_drbg,
        mbedtls_entropy_func,
        &entropy,
        (const unsigned char*)pers,
        strlen(pers));


    uint8_t digest[32];
    rc = mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), headerAndPayload, strlen((char*)headerAndPayload), digest);
    if (rc != 0) {
        printf("Failed to mbedtls_md: %d (-0x%x): %s\n", rc, -rc, mbedtlsError(rc));
        return 0;
    }

    size_t retSize;
    rc = mbedtls_pk_sign(&pk_context, MBEDTLS_MD_SHA256, digest, sizeof(digest), oBuf, &retSize, mbedtls_ctr_drbg_random, &ctr_drbg);
    if (rc != 0) {
        printf("Failed to mbedtls_pk_sign: %d (-0x%x): %s\n", rc, -rc, mbedtlsError(rc));
        return 0;
    }


    char base64Signature[600];
    base64url_encode((unsigned char *)oBuf, retSize, base64Signature);

    char* retData = (char*)malloc(strlen((char*)headerAndPayload) + 1 + strlen((char*)base64Signature) + 1);

    sprintf(retData, "%s.%s", headerAndPayload, base64Signature);

    mbedtls_pk_free(&pk_context);
    return retData;
}


