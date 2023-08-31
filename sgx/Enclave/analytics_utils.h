#include <sgx_trts.h>
#include <sgx_tseal.h>
#include "sgx_tcrypto.h"
#include <map>
#include <string>
#include "EnclaveHelper.h"

#define BUFLEN 2048
#define SGX_AESGCM_MAC_SIZE 16
#define SGX_AESGCM_IV_SIZE 12

#ifndef IOTENCLAVE_ANALYTICS_UTILS_H
#define IOTENCLAVE_ANALYTICS_UTILS_H


bool encryptMessage_AES_GCM_Tag(char *decMessageIn, size_t len, char *encMessageOut, size_t lenOut, char *tagMessageOut);
bool decryptMessage_AES_GCM_Tag(char *encMessageIn, size_t len, char *decMessageOut, size_t lenOut, char *tag);

bool encryptMessage_AES_GCM(char *decMessageIn, size_t len, char *encMessageOut, size_t lenOut);
bool decryptMessage_AES_GCM(char *encMessageIn, size_t len, char *decMessageOut, size_t lenOut);

#endif //IOTENCLAVE_ANALYTICS_UTILS_H

