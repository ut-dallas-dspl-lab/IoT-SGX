#include <inttypes.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <main_TA.h>
#include <device_manager_TA.h>
#include <parser_TA.h>
#include <string.h>

#define AES128_KEY_BIT_SIZE		128
#define AES128_KEY_BYTE_SIZE		(AES128_KEY_BIT_SIZE / 8)
#define AES256_KEY_BIT_SIZE		256
#define AES256_KEY_BYTE_SIZE		(AES256_KEY_BIT_SIZE / 8)

/*
 * Ciphering context: each opened session relates to a cipehring operation.
 * - configure the AES flavour from a command.
 * - load key from a command (here the key is provided by the REE)
 * - reset init vector (here IV is provided by the REE)
 * - cipher a buffer frame (here input and output buffers are non-secure)
 */
struct aes_cipher {
	uint32_t algo;			/* AES flavour */
	uint32_t mode;			/* Encode or decode */
	uint32_t key_size;		/* AES key size in byte */
	TEE_OperationHandle op_handle;	/* AES ciphering operation */
	TEE_ObjectHandle key_handle;	/* transient object to load the key */
};


/* Default Key, IV, AAD */
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


char TRIGGER_EVENT_BUFFER[TA_TRIGGER_EVENT_SIZE]; /* Stores encrypted trigger event */
char TRIGGER_EVENT_TEMPLATE[] = "{\"deviceID\": \"%s\", \"%s\": {\"%s\": {\"value\": \"%s\", \"unit\": \"#\", \"timestamp\": \"%ld\"}}}"; /* Trigger event basic template */

/*
 * Few routines to convert IDs from TA API into IDs from OP-TEE.
 */
static TEE_Result ta2tee_algo_id(uint32_t param, uint32_t *algo)
{
	switch (param) {
	case TA_AES_ALGO_ECB:
		*algo = TEE_ALG_AES_ECB_NOPAD;
		return TEE_SUCCESS;
	case TA_AES_ALGO_CBC:
		*algo = TEE_ALG_AES_CBC_NOPAD;
		return TEE_SUCCESS;
	case TA_AES_ALGO_CTR:
		*algo = TEE_ALG_AES_CTR;
		return TEE_SUCCESS;
    case TA_AES_ALGO_GCM:
        *algo = TEE_ALG_AES_GCM;
        return TEE_SUCCESS;
	default:
		EMSG("Invalid algo %u", param);
		return TEE_ERROR_BAD_PARAMETERS;
	}
}
static TEE_Result ta2tee_key_size(uint32_t param, uint32_t *key_size)
{
	switch (param) {
	case AES128_KEY_BYTE_SIZE:
	case AES256_KEY_BYTE_SIZE:
		*key_size = param;
		return TEE_SUCCESS;
	default:
		EMSG("Invalid key size %u", param);
		return TEE_ERROR_BAD_PARAMETERS;
	}
}
static TEE_Result ta2tee_mode_id(uint32_t param, uint32_t *mode)
{
	switch (param) {
	case TA_AES_MODE_ENCODE:
		*mode = TEE_MODE_ENCRYPT;
		return TEE_SUCCESS;
	case TA_AES_MODE_DECODE:
		*mode = TEE_MODE_DECRYPT;
		return TEE_SUCCESS;
	default:
		EMSG("Invalid mode %u", param);
		return TEE_ERROR_BAD_PARAMETERS;
	}
}

/*
 * Process command TA_CMD_AES_GCM_PREPARE. API in main_TA.h
 */
static TEE_Result alloc_resources_TA(void *session, uint32_t param_types,
                                  TEE_Param params[4])
{
    const uint32_t exp_param_types =
            TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                            TEE_PARAM_TYPE_VALUE_INPUT,
                            TEE_PARAM_TYPE_VALUE_INPUT,
                            TEE_PARAM_TYPE_VALUE_INPUT);
    struct aes_cipher *sess;
    TEE_Attribute attr;
    TEE_Result res;
    char *key;

    /* Get ciphering context from session ID */
    DMSG("Session %p: get ciphering resources", session);
    sess = (struct aes_cipher *)session;

    /* Safely get the invocation parameters */
    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    res = ta2tee_algo_id(params[0].value.a, &sess->algo);
    if (res != TEE_SUCCESS)
        return res;

    res = ta2tee_key_size(params[1].value.a, &sess->key_size);
    if (res != TEE_SUCCESS)
        return res;

    res = ta2tee_mode_id(params[2].value.a, &sess->mode);
    if (res != TEE_SUCCESS)
        return res;

    int is_key_loaded = params[3].value.a;

    /*
     * Ready to allocate the resources which are:
     * - an operation handle, for an AES ciphering of given configuration
     * - a transient object that will be use to load the key materials
     *   into the AES ciphering operation.
     */

    /* Free potential previous operation */
    if (sess->op_handle != TEE_HANDLE_NULL)
        TEE_FreeOperation(sess->op_handle);

    /* Allocate operation: AES/CTR, mode and size from params */
    res = TEE_AllocateOperation(&sess->op_handle,
                                sess->algo,
                                sess->mode,
                                sess->key_size * 8);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to allocate operation");
        sess->op_handle = TEE_HANDLE_NULL;
        goto err;
    }

    /* Free potential previous transient object */
    if (sess->key_handle != TEE_HANDLE_NULL)
        TEE_FreeTransientObject(sess->key_handle);

    /* Allocate transient object according to target key size */
    res = TEE_AllocateTransientObject(TEE_TYPE_AES,
                                      sess->key_size * 8,
                                      &sess->key_handle);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to allocate transient object");
        sess->key_handle = TEE_HANDLE_NULL;
        goto err;
    }

    if(is_key_loaded) {
        /* KEY */
        key = gcm_key;
        uint32_t key_sz = sizeof(gcm_key);

        //IMSG("Loading Key...");
        if (key_sz != sess->key_size) {
            EMSG("Wrong key size %" PRIu32 ", expect %" PRIu32 " bytes",
                 key_sz, sess->key_size);
            return TEE_ERROR_BAD_PARAMETERS;
        }

        TEE_InitRefAttribute(&attr, TEE_ATTR_SECRET_VALUE, key, key_sz);

        //TEE_ResetTransientObject(sess->key_handle);
        res = TEE_PopulateTransientObject(sess->key_handle, &attr, 1);
        if (res != TEE_SUCCESS) {
            EMSG("TEE_PopulateTransientObject failed, %x", res);
            return res;
        }

        //TEE_ResetOperation(sess->op_handle);
        res = TEE_SetOperationKey(sess->op_handle, sess->key_handle);
        if (res != TEE_SUCCESS) {
            EMSG("TEE_SetOperationKey failed %x", res);
            return res;
        }

        /* IV */
        char* iv = gcm_iv;
        uint32_t iv_sz = sizeof(gcm_iv);
        //IMSG("Loading IV...");
        TEE_AEInit(sess->op_handle, iv, iv_sz, TA_AES_GCM_TAG_SIZE * 8, 0, 0);

        /* AAD */
        char* aad = gcm_aad;
        uint32_t aad_sz = sizeof(gcm_aad);
        //IMSG("Loading AAD...");
        TEE_AEUpdateAAD(sess->op_handle, aad, aad_sz);
    }
    else{
        /*
        * When loading a key in the cipher session, set_aes_key()
        * will reset the operation and load a key. But we cannot
        * reset and operation that has no key yet (GPD TEE Internal
        * Core API Specification – Public Release v1.1.1, section
        * 6.2.5 TEE_ResetOperation). In consequence, we will load a
        * dummy key in the operation so that operation can be reset
        * when updating the key.
        */
        key = TEE_Malloc(sess->key_size, 0);
        if (!key) {
            res = TEE_ERROR_OUT_OF_MEMORY;
            goto err;
        }

        TEE_InitRefAttribute(&attr, TEE_ATTR_SECRET_VALUE, key, sess->key_size);

        res = TEE_PopulateTransientObject(sess->key_handle, &attr, 1);
        if (res != TEE_SUCCESS) {
            EMSG("TEE_PopulateTransientObject failed, %x", res);
            goto err;
        }

        res = TEE_SetOperationKey(sess->op_handle, sess->key_handle);
        if (res != TEE_SUCCESS) {
            EMSG("TEE_SetOperationKey failed %x", res);
            goto err;
        }
    }

    return res;

    err:
    if (sess->op_handle != TEE_HANDLE_NULL)
        TEE_FreeOperation(sess->op_handle);
    sess->op_handle = TEE_HANDLE_NULL;

    if (sess->key_handle != TEE_HANDLE_NULL)
        TEE_FreeTransientObject(sess->key_handle);
    sess->key_handle = TEE_HANDLE_NULL;

    return res;
}

/*
 * Process command TA_CMD_AES_GCM_ENCRYPT. API in main_TA.h
 */
static TEE_Result encrypt_final_TA(void *session, uint32_t param_types,
                                TEE_Param params[4])
{
    const uint32_t exp_param_types =
            TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                            TEE_PARAM_TYPE_MEMREF_OUTPUT,
                            TEE_PARAM_TYPE_NONE,
                            TEE_PARAM_TYPE_NONE);
    struct aes_cipher *sess;

    /* Get ciphering context from session ID */
    DMSG("Session %p: encrypt_final", session);
    sess = (struct aes_cipher *)session;

    /* Safely get the invocation parameters */
    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;
    if (sess->op_handle == TEE_HANDLE_NULL)
        return TEE_ERROR_BAD_STATE;

    char *cipher = params[1].memref.buffer;
    size_t cipher_size = params[1].memref.size;

    size_t cipher_size_wo_tag = cipher_size - TA_AES_GCM_TAG_SIZE;
    char *cipher_wo_tag = TEE_Malloc(cipher_size_wo_tag, 0);

    char *tag = TEE_Malloc(TA_AES_GCM_TAG_SIZE, 0);
    size_t tag_size = TA_AES_GCM_TAG_SIZE;

    /*
     * Process ciphering operation on provided buffers
     */
    TEE_Result res = TEE_AEEncryptFinal(sess->op_handle,
                                        params[0].memref.buffer, params[0].memref.size,
                                        cipher_wo_tag, &cipher_size_wo_tag,
                                        tag, &tag_size);

    if (res != TEE_SUCCESS){
        IMSG("ERROR! Encrypting response failed!");
        return TEE_ERROR_GENERIC;
    }

    TEE_MemMove(cipher, tag, TA_AES_GCM_TAG_SIZE);
    TEE_MemMove(cipher + TA_AES_GCM_TAG_SIZE, cipher_wo_tag, cipher_size_wo_tag);

    TEE_Free(cipher_wo_tag);
    TEE_Free(tag);
    return TEE_SUCCESS;
}

/*
 * Process command TA_CMD_AES_GCM_DECRYPT. API in main_TA.h
 */
static TEE_Result decrypt_final_TA(void *session, uint32_t param_types,
                                TEE_Param params[4])
{
    const uint32_t exp_param_types =
            TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                            TEE_PARAM_TYPE_MEMREF_OUTPUT,
                            TEE_PARAM_TYPE_NONE,
                            TEE_PARAM_TYPE_NONE);
    struct aes_cipher *sess;

    /* Get ciphering context from session ID */
    DMSG("Session %p: decrypt_final", session);
    sess = (struct aes_cipher *)session;

    /* Safely get the invocation parameters */
    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;
    if (sess->op_handle == TEE_HANDLE_NULL)
        return TEE_ERROR_BAD_STATE;

    /* Copying data into TEE before calling TEE_AEDecryptFinal. See issue: https://github.com/OP-TEE/optee_os/issues/5946 */
    char *cipher = params[0].memref.buffer;
    size_t cipher_size = params[0].memref.size;

    size_t plaintext_size = params[1].memref.size;

    char *plaintext = TEE_Malloc(plaintext_size, 0);
    char *cipher_wo_tag = TEE_Malloc(plaintext_size, 0);
    char *tag = TEE_Malloc(TA_AES_GCM_TAG_SIZE, 0);
    size_t tag_size = TA_AES_GCM_TAG_SIZE;

    TEE_MemMove(tag, cipher, TA_AES_GCM_TAG_SIZE);
    TEE_MemMove(cipher_wo_tag, cipher+TA_AES_GCM_TAG_SIZE, plaintext_size);

    /*
     * Process ciphering operation on provided buffers
     */
    TEE_Result res = TEE_AEDecryptFinal(sess->op_handle,
                                        cipher_wo_tag, plaintext_size,
                                        plaintext, &plaintext_size,
                                        tag, TA_AES_GCM_TAG_SIZE);

    if (res != TEE_SUCCESS){
        IMSG("ERROR! Decryption failed!");
        return TEE_ERROR_GENERIC;
    }

    TEE_MemMove(params[1].memref.buffer, plaintext, plaintext_size);

    TEE_Free(cipher_wo_tag);
    TEE_Free(tag);
    TEE_Free(plaintext);
    return res;
}



/*************************************************************
    * IoT Side
**************************************************************/

/*
 * Process command TA_CMD_SETUP_DEVICE_INFO. API in main_TA.h
 */
static TEE_Result setup_device_info_TA(void *session, uint32_t param_types,
                                       TEE_Param params[4]) {
    const uint32_t exp_param_types =
            TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                            TEE_PARAM_TYPE_NONE,
                            TEE_PARAM_TYPE_NONE,
                            TEE_PARAM_TYPE_NONE);
    //DMSG("has been called");

    /* Safely get the invocation parameters */
    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    char *info = params[0].memref.buffer;
    int ret = prepare_device_info(info);
    if (ret == 0){
        IMSG("ERROR! setup_device_info failed!");
        return TEE_ERROR_GENERIC;
    }
    return TEE_SUCCESS;
}

/*
 * Process command TA_CMD_NUM_SUBSCRIBER_DEVICES. API in main_TA.h
 */
static TEE_Result get_num_subscriber_devices_TA(void *session, uint32_t param_types,
                                    TEE_Param params[4]) {
    const uint32_t exp_param_types =
            TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_OUTPUT,
                            TEE_PARAM_TYPE_NONE,
                            TEE_PARAM_TYPE_NONE,
                            TEE_PARAM_TYPE_NONE);
    //DMSG("has been called");

    /* Safely get the invocation parameters */
    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    params[0].value.a = get_num_subscriber_devices();
    return TEE_SUCCESS;
}

/*
 * Process command TA_CMD_SUBSCRIBER_TOPICS. API in main_TA.h
 */
static TEE_Result get_subscriber_topics_TA(void *session, uint32_t param_types,
                                                TEE_Param params[4]) {
    const uint32_t exp_param_types =
            TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                            TEE_PARAM_TYPE_MEMREF_OUTPUT,
                            TEE_PARAM_TYPE_NONE,
                            TEE_PARAM_TYPE_NONE);
    //DMSG("has been called");

    /* Safely get the invocation parameters */
    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    int sub_topic_index = params[0].value.a;
    char *mqtt_topic = params[1].memref.buffer;

    int ret = get_subscriber_topic(sub_topic_index, &mqtt_topic);
    if (ret == 0){
        IMSG("Topic name not found!");
        return TEE_ERROR_ITEM_NOT_FOUND;
    }
    return TEE_SUCCESS;
}


/*
 * Process command TA_CMD_ENCRYPT_TRIGGER_EVENT. API in main_TA.h
 */
static TEE_Result encrypt_sensor_value_TA(void *session, uint32_t param_types,
                                       TEE_Param params[4]){
    const uint32_t exp_param_types =
            TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                            TEE_PARAM_TYPE_MEMREF_OUTPUT,
                            TEE_PARAM_TYPE_MEMREF_OUTPUT,
                            TEE_PARAM_TYPE_VALUE_INPUT);
    struct aes_cipher *sess;
    /* Get ciphering context from session ID */
    DMSG("Session %p: encrypt_sensor_value", session);
    sess = (struct aes_cipher *)session;

    /* Safely get the invocation parameters */
    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    if (params[1].memref.size < params[0].memref.size) {
        EMSG("Bad sizes: in %d, out %d", params[0].memref.size,
             params[1].memref.size);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    TEE_Result res;

    char *sensor_value = params[0].memref.buffer;
    char *response = params[1].memref.buffer;
    char *mqtt_topic = params[2].memref.buffer;
    int device_type = params[3].value.a;
    int mqtt_op_type = params[3].value.b;

    size_t response_size = params[1].memref.size; // this includes response length + tag length
    size_t response_size_wo_tag = response_size - TA_AES_GCM_TAG_SIZE;
    //IMSG("response_size=%d, response_size_wo_tag=%d",response_size,response_size_wo_tag);

    char *response_wo_tag = TEE_Malloc(response_size_wo_tag, 0);
    char *response_wo_tag_enc = TEE_Malloc(response_size_wo_tag, 0);
    char *tag = TEE_Malloc(TA_AES_GCM_TAG_SIZE, 0);
    size_t tag_size = TA_AES_GCM_TAG_SIZE;


    /* Prepare Trigger Event */
    int isSuccess = prepare_trigger_event(sensor_value, device_type, mqtt_op_type, &response_wo_tag, &mqtt_topic);
    if (isSuccess == 0){
        IMSG("ERROR! Preparing trigger event failed!");
        res = TEE_ERROR_GENERIC;
        goto end;
    }
    IMSG("Trigger: %s, length=%d\n", response_wo_tag, strlen(response_wo_tag));

    /* Encrypt Trigger Event */
    res = TEE_AEEncryptFinal(sess->op_handle,
                       response_wo_tag, response_size_wo_tag,
                       response_wo_tag_enc, &response_size_wo_tag,
                       tag, &tag_size);

    if (res != TEE_SUCCESS){
        IMSG("ERROR! Encrypting response failed!");
        goto end;
    }
    //IMSG("Done Encryption!");

    TEE_MemMove(response, tag, TA_AES_GCM_TAG_SIZE);
    TEE_MemMove(response + TA_AES_GCM_TAG_SIZE, response_wo_tag_enc, response_size_wo_tag);

    end:
    TEE_Free(response_wo_tag);
    TEE_Free(response_wo_tag_enc);
    TEE_Free(tag);

    return res;
}

/*
 * Process command TA_CMD_DECRYPT_ACTION_EVENT. API in main_TA.h
 */
static TEE_Result decrypt_action_command_TA(void *session, uint32_t param_types,
                                         TEE_Param params[4]){
    const uint32_t exp_param_types =
            TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                            TEE_PARAM_TYPE_MEMREF_OUTPUT,
                            TEE_PARAM_TYPE_MEMREF_OUTPUT,
                            TEE_PARAM_TYPE_NONE);
    struct aes_cipher *sess;
    /* Get ciphering context from session ID */
    DMSG("Session %p: decrypt_action_command", session);
    sess = (struct aes_cipher *)session;

    /* Safely get the invocation parameters */
    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;
    if (sess->op_handle == TEE_HANDLE_NULL)
        return TEE_ERROR_BAD_STATE;

    char *action_command = params[0].memref.buffer;
    size_t action_command_size = params[0].memref.size; // this includes length + tag length

    size_t action_size_wo_tag = action_command_size - TA_AES_GCM_TAG_SIZE;
    IMSG("action_command=%d, action_size_wo_tag=%d", action_command_size,action_size_wo_tag);

    char *action_wo_tag = TEE_Malloc(action_size_wo_tag, 0);
    char *action_wo_tag_enc = TEE_Malloc(action_size_wo_tag, 0);
    char *aes_tag = TEE_Malloc(TA_AES_GCM_TAG_SIZE, 0);
    size_t tag_size = TA_AES_GCM_TAG_SIZE;

    TEE_MemMove(aes_tag, action_command, TA_AES_GCM_TAG_SIZE);
    TEE_MemMove(action_wo_tag_enc, action_command+TA_AES_GCM_TAG_SIZE, action_size_wo_tag);

    /* Decrypt Action Command Event */
    TEE_Result res = TEE_AEDecryptFinal(sess->op_handle,
                                        action_wo_tag_enc, action_size_wo_tag,
                                        action_wo_tag, &action_size_wo_tag,
                                        aes_tag, TA_AES_GCM_TAG_SIZE);

    if (res != TEE_SUCCESS){
        IMSG("ERROR! Decryption failed!");
        goto end;
    }
    IMSG("Done Decryption: %s", action_wo_tag);

    int device_type = -1, command_type = -1, argument_type=-1, argument_size=-1;
    char *argument = params[2].memref.buffer;
    int *features = params[1].memref.buffer;

    /* Parse Action Command */
    int isSuccess = prepare_action_command(action_wo_tag, &device_type, &command_type, &argument_type, &argument, &argument_size);
    if (isSuccess == 0){
        res = TEE_ERROR_GENERIC;
        goto end;
    }

    /* Populate output buffer with macro commands */
    features[0] = device_type;
    features[1] = command_type;
    features[2] = argument_type;
    features[3] = argument_size;

    end:
    TEE_Free(aes_tag);
    TEE_Free(action_wo_tag_enc);
    TEE_Free(action_wo_tag);

    return res;
}



/*************************************************************
    * Initialize TEE
**************************************************************/

TEE_Result TA_CreateEntryPoint(void)
{
	/* Nothing to do */
    DMSG("has been called");
	return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{
	/* Nothing to do */
    DMSG("has been called");
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t __unused param_types, TEE_Param __unused params[4], void __unused **session)
{
	struct aes_cipher *sess;

	/*
	 * Allocate and init materials for the session.
	 * The address of the structure is used as session ID for
	 * the client.
	 */
	sess = TEE_Malloc(sizeof(*sess), 0);
	if (!sess)
		return TEE_ERROR_OUT_OF_MEMORY;

	sess->key_handle = TEE_HANDLE_NULL;
	sess->op_handle = TEE_HANDLE_NULL;

	*session = (void *)sess;
	DMSG("Session %p: newly allocated", *session);

	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void *session)
{
	struct aes_cipher *sess;

	/* Get ciphering context from session ID */
	DMSG("Session %p: release session", session);
	sess = (struct aes_cipher *)session;

    /* Free resources */
    destroy_device_network_info();

	/* Release the session resources */
	if (sess->key_handle != TEE_HANDLE_NULL)
		TEE_FreeTransientObject(sess->key_handle);
	if (sess->op_handle != TEE_HANDLE_NULL)
		TEE_FreeOperation(sess->op_handle);
	TEE_Free(sess);
}





TEE_Result TA_InvokeCommandEntryPoint(void *session,
					uint32_t cmd,
					uint32_t param_types,
					TEE_Param params[4])
{
    switch (cmd) {
        case TA_CMD_AES_GCM_PREPARE:
            return alloc_resources_TA(session, param_types, params);
        case TA_CMD_AES_GCM_ENCRYPT:
            return encrypt_final_TA(session, param_types, params);
        case TA_CMD_AES_GCM_DECRYPT:
            return decrypt_final_TA(session, param_types, params);
        case TA_CMD_SETUP_DEVICE_INFO:
            return setup_device_info_TA(session, param_types, params);
        case TA_CMD_NUM_SUBSCRIBER_DEVICES:
            return get_num_subscriber_devices_TA(session, param_types, params);
        case TA_CMD_SUBSCRIBER_TOPICS:
            return get_subscriber_topics_TA(session, param_types, params);
        case TA_CMD_ENCRYPT_TRIGGER_EVENT:
            return encrypt_sensor_value_TA(session, param_types, params);
        case TA_CMD_DECRYPT_ACTION_EVENT:
            return decrypt_action_command_TA(session, param_types, params);
        default:
            EMSG("Command ID 0x%x is not supported", cmd);
            return TEE_ERROR_NOT_SUPPORTED;
    }
}
