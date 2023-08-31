#include "analytics_utils.h"
#include <stdio.h>
//#include <cstring>
#include <string.h>
#include <algorithm>

#include "Enclave.h"
#include "Enclave_t.h"
#include "ObliviousOperationManager.h"

static sgx_aes_gcm_128bit_key_t key2 = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };

static const unsigned char gcm_key[] = {
        0xee, 0xbc, 0x1f, 0x57, 0x48, 0x7f, 0x51, 0x92, 0x1c, 0x04, 0x65, 0x66, 0x5f, 0x8a, 0xe6, 0xd1
};
static const unsigned char gcm_iv[] = {
        0x99, 0xaa, 0x3e, 0x68, 0xed, 0x81, 0x73, 0xa0, 0xee, 0xd0, 0x66, 0x84
};
static const unsigned char gcm_aad[] = {
        0x4d, 0x23, 0xc3, 0xce, 0xc3, 0x34, 0xb4, 0x9b, 0xdb, 0x37, 0x0c, 0x43,
        0x7f, 0xec, 0x78, 0xde
};

char dummyTag[] = "DummyTag12345678";


/*
 * decryptMessage_AES_GCM_Tag:
 *  @param encMessageIn: encrypted message
 *  @param len: encrypted message length
 *  @param decMessageOut: decrypted message
 *  @param lenOut: decrypted message length
 *  @param tag: required tag/mac for AES GCM cipher
 *  returns: true if successful, else false
 */
bool decryptMessage_AES_GCM_Tag(char *encMessageIn, size_t len, char *decMessageOut, size_t lenOut, char *tag){
    if(IS_DEBUG) printf("Started Decryption.....");
    //printf("\n### Data, tag: \n %s\n %s\n", encMessageIn, tag);
    //printf("### Data, tag sizes: \n %ld\n %ld\n", len, strlen(tag));
    sgx_aes_gcm_128bit_key_t *key = (sgx_aes_gcm_128bit_key_t*)gcm_key;
    uint8_t *iv = (uint8_t *)gcm_iv;
    uint8_t *aad = (uint8_t *)gcm_aad;
    uint8_t  *encMessage = (uint8_t *)encMessageIn;
    uint8_t p_dst[BUFLEN] = {0};

    sgx_status_t stat = sgx_rijndael128GCM_decrypt(key, encMessage, len, p_dst, iv, SGX_AESGCM_IV_SIZE, aad, SGX_AESGCM_MAC_SIZE, (sgx_aes_gcm_128bit_tag_t *) tag );

    bool isDecryptionSuccess = obliviousSelectEq(stat, 0);
    if(IS_DEBUG) check_error_code(stat);

    isDecryptionSuccess ? o_memcpy(1, decMessageOut, p_dst, lenOut) : o_memcpy(1, decMessageOut, encMessageIn, lenOut);
    if(IS_DEBUG && isDecryptionSuccess) printf("### Decrypted message: %s with length %ld\n", decMessageOut, strlen(decMessageOut));

    return isDecryptionSuccess;
}

/*
 * decryptMessage_AES_GCM:
 *  @param encMessageIn: encrypted message, which contains tag/mac
 *  @param len: encrypted message length
 *  @param decMessageOut: decrypted message
 *  @param lenOut: decrypted message length
 *  returns: true if successful, else false
 */
bool decryptMessage_AES_GCM(char *encMessageIn, size_t len, char *decMessageOut, size_t lenOut){
    char *tag = (char*) malloc((SGX_AESGCM_MAC_SIZE+1) * sizeof(char));
    memcpy(tag, encMessageIn, SGX_AESGCM_MAC_SIZE);
    tag[SGX_AESGCM_MAC_SIZE] = '\0';

    size_t enc_msg_len = len - SGX_AESGCM_MAC_SIZE;
    char *enc_msg = (char*) malloc((enc_msg_len+1) * sizeof(char));
    memcpy(enc_msg, encMessageIn+SGX_AESGCM_MAC_SIZE, enc_msg_len);
    enc_msg[enc_msg_len] = '\0';

    bool status = decryptMessage_AES_GCM_Tag(enc_msg, enc_msg_len, decMessageOut, lenOut, tag);

    if (enc_msg != NULL )free(enc_msg);
    if (tag != NULL) free(tag);
    return status;
}


/*
 * encryptMessage_AES_GCM_Tag:
 *  @param decMessageIn: plaintext message
 *  @param len: plaintext message length
 *  @param encMessageOut: encrypted message
 *  @param lenOut: encrypted message length
 *  @param tagMessageOut: generated tag/mac of AES GCM cipher
 *  returns: true if successful, else false
 */
bool encryptMessage_AES_GCM_Tag(char *decMessageIn, size_t len, char *encMessageOut, size_t lenOut, char *tagMessageOut){
    if(IS_DEBUG) printf("Started Encryption.....");
    sgx_aes_gcm_128bit_key_t *key = (sgx_aes_gcm_128bit_key_t*)gcm_key;
    uint8_t *iv = (uint8_t *)gcm_iv;
    uint8_t *aad = (uint8_t *)gcm_aad;
    uint8_t  *decMessage = (uint8_t *)decMessageIn;
    uint8_t p_dst[lenOut] = {0};
    uint8_t p_dst2[SGX_AESGCM_MAC_SIZE] = {0};

    sgx_status_t stat = sgx_rijndael128GCM_encrypt(key, decMessage, len, p_dst, iv, SGX_AESGCM_IV_SIZE, aad, SGX_AESGCM_MAC_SIZE, (sgx_aes_gcm_128bit_tag_t *) p_dst2);

    bool isEncryptionSuccess =  obliviousSelectEq(stat, 0);
    if(IS_DEBUG) check_error_code(stat);

    isEncryptionSuccess ? o_memcpy(1,encMessageOut, p_dst, lenOut) : o_memcpy(1, encMessageOut, decMessageIn, lenOut);
    isEncryptionSuccess ? o_memcpy(1, tagMessageOut, p_dst2, SGX_AESGCM_MAC_SIZE) : o_memcpy(1, tagMessageOut, dummyTag, SGX_AESGCM_MAC_SIZE);
    if(IS_DEBUG && isEncryptionSuccess) printf("### Encryption Success, with length of message=%ld & tag=%ld\n", strlen(encMessageOut), strlen(tagMessageOut));

    return isEncryptionSuccess;
}

/*
 * encryptMessage_AES_GCM:
 *  @param decMessageIn: plaintext message
 *  @param len: plaintext message length
 *  @param encMessageOut: encrypted message, which contains generated tag/mac
 *  @param lenOut: encrypted message length
 *  returns: true if successful, else false
 */
bool encryptMessage_AES_GCM(char *decMessageIn, size_t len, char *encMessageOut, size_t lenOut){
    char *tag = (char*) malloc((SGX_AESGCM_MAC_SIZE+1) * sizeof(char));
    size_t enc_msg_len = lenOut - SGX_AESGCM_MAC_SIZE;
    char *enc_msg = (char*) malloc((enc_msg_len+1) * sizeof(char));

    bool status = encryptMessage_AES_GCM_Tag(decMessageIn, len, enc_msg, enc_msg_len, tag);

    memcpy(encMessageOut, tag, SGX_AESGCM_MAC_SIZE);
    memcpy(encMessageOut+SGX_AESGCM_MAC_SIZE, enc_msg, enc_msg_len);
    encMessageOut[lenOut] = '\0';

    free(enc_msg);
    free(tag);
    return status;
}


