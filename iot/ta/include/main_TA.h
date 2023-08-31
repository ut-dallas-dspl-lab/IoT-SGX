#ifndef OPTEE_EXAMPLE_IOT_MAIN_TA_H
#define OPTEE_EXAMPLE_IOT_MAIN_TA_H

/* UUID of the trusted application */
#define TA_IOT_UUID \
	{ 0x5dbac793, 0xf574, 0x4871, \
		{ 0x8a, 0xd3, 0x04, 0x33, 0x1e, 0xc1, 0x7e, 0x23 } }


/**********************************************
 * Macro: AES
 *********************************************/

#define TA_AES_ALGO_ECB			0
#define TA_AES_ALGO_CBC			1
#define TA_AES_ALGO_CTR			2
#define TA_AES_ALGO_GCM			3

#define TA_AES_SIZE_128BIT		(128 / 8)
#define TA_AES_SIZE_256BIT		(256 / 8)
#define TA_AES_GCM_TAG_SIZE		16

#define TA_AES_MODE_DECODE		0
#define TA_AES_MODE_ENCODE		1


/**********************************************
 * Macro: TA Entry Points
 *********************************************/
/*
 * TA_AES_CMD_PREPARE - Allocate resources for the AES ciphering
 * param[0] (value) a: TA_AES_ALGO_xxx, b: unused
 * param[1] (value) a: key size in bytes, b: unused
 * param[2] (value) a: TA_AES_MODE_ENCODE/_DECODE, b: unused
 * param[3] (value) a: is_key_loaded=1 if key/iv is already loaded in the TA, is_key_loaded=0 otherwise; b: unused
 */
#define TA_CMD_AES_GCM_PREPARE		0
/*
 * TA_CMD_AES_GCM_ENCRYPT - Cipher input buffer into output buffer
 * param[0] (memref): input buffer containing plaintext
 * param[1] (memref): output buffer containing ciphertext
 * param[2] unused
 * param[3] unused
 */
#define TA_CMD_AES_GCM_ENCRYPT		1
/*
 * TA_CMD_AES_GCM_DECRYPT - Cipher input buffer into output buffer
 * param[0] (memref): input buffer containing ciphertext
 * param[1] (memref): output buffer containing plaintext
 * param[2] unused
 * param[3] unused
 */
#define TA_CMD_AES_GCM_DECRYPT		2
/*
 * TA_CMD_SETUP_DEVICE_INFO - Setup device information in SW
 * param[0] (memref): input buffer containing device information
 * param[1] unused
 * param[2] unused
 * param[3] unused
 */
#define TA_CMD_SETUP_DEVICE_INFO	3
/*
 * TA_CMD_NUM_SUBSCRIBER_DEVICES - Get number of devices with sub topics
 * param[0] (value) a: number of devices
 * param[1] unused
 * param[2] unused
 * param[3] unused
 */
#define TA_CMD_NUM_SUBSCRIBER_DEVICES	4
/*
 * TA_CMD_SUBSCRIBER_TOPICS - Get subscriber topic name for given device index
 * param[0] (value) a: device index
 * param[1] (memref): output buffer containing topic name
 * param[2] unused
 * param[3] unused
 */
#define TA_CMD_SUBSCRIBER_TOPICS		5
/*
 * TA_CMD_ENCRYPT_TRIGGER_EVENT - Prepare encrypted trigger event from device value for sending to Server
 * param[0] (memref): input buffer containing trigger value
 * param[1] (memref): output buffer containing encrypted trigger event
 * param[2] (memref): output buffer containing mqtt publish topic name
 * param[3] (value) a: input macro of device type; b: input macro of mqtt operation type
 */
#define TA_CMD_ENCRYPT_TRIGGER_EVENT	6
/*
 * TA_CMD_DECRYPT_ACTION_EVENT - Prepare and parse action command received from server
 * param[0] (memref): input buffer containing encrypted action command
 * param[1] (memref): output buffer containing action command features (an array of macros sized TA_ACTION_COMMAND_FEATURE_SIZE)
 * param[2] (memref): output buffer containing action command additional arguments
 * param[3] unused
 */
#define TA_CMD_DECRYPT_ACTION_EVENT		7



/**********************************************
 * Macro: MQTT Pub/Sub
 *********************************************/
#define TA_MQTT_OPERATION_PUBLISH		0
#define TA_MQTT_OPERATION_SUBSCRIBE		1
#define TA_MQTT_OPERATION_BOTH		2


/**********************************************
 * Macro: Sensor/Actuator
 *********************************************/

/* Device type/capability */
#define TA_DEVICE_TYPE_SWITCH		            0
#define TA_DEVICE_TYPE_LIGHT		            1
#define TA_DEVICE_TYPE_LED		                2
#define TA_DEVICE_TYPE_SENSOR_MOTION		    3
#define TA_DEVICE_TYPE_SENSOR_TEMPERATURE		4
#define TA_DEVICE_TYPE_SENSOR_HUMIDITY		    5

/* Actuator action-command type */
#define TA_ACTUATOR_CMD_OFF		            0
#define TA_ACTUATOR_CMD_ON		            1
#define TA_ACTUATOR_CMD_SET		            2

/* Actuator additional argument type */
#define TA_ACTUATOR_ARG_SLEEP		            0
#define TA_ACTUATOR_ARG_LEVEL		            1
#define TA_ACTUATOR_ARG_MODE		            2


/**********************************************
 * Macro: Misc.
 *********************************************/
#define TA_BUFFER_SIZE                  512
#define TA_TRIGGER_EVENT_SIZE		    512
#define TA_ACTION_COMMAND_SIZE		    4096
#define TA_ACTION_COMMAND_FEATURE_SIZE    4


/**********************************************
 * Structs & Enums Definition
 *********************************************/

enum ValueType {STRING, NUMBER};


/* IoT device info */
typedef struct iot{
    char *deviceID;
    char *capability;
    char *attribute;
    char *unit;
    char *value;
    enum ValueType value_type;
    int device_type;
    char *mqtt_topic;
    int mqtt_op_type;
    /* TODO: add other device properties */
} iot;

/* IoT network of a user */
typedef struct network{
    char *userID;
    char *locationID;
    iot *devices;
    int n;
} network;


/**********************************************
 * Globals
 *********************************************/
extern char TRIGGER_EVENT_BUFFER[TA_TRIGGER_EVENT_SIZE];
extern char TRIGGER_EVENT_TEMPLATE[TA_TRIGGER_EVENT_SIZE];

#endif /* OPTEE_EXAMPLE_IOT_MAIN_TA_H */
