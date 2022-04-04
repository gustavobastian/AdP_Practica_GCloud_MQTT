/*
 * jwt_token_gcp.h
 *
 *  Created on: Jul 16, 2020
 *      Author: leopoldo
 */

#ifndef MAIN_JWT_TOKEN_GCP_H_
#define MAIN_JWT_TOKEN_GCP_H_

#include <stdint.h>
#include <stdlib.h>

char* createGCPJWT(char* projectId, unsigned char* privateKey, size_t privateKeySize, uint16_t expiration_minutes);

#endif /* MAIN_JWT_TOKEN_GCP_H_ */
