#include "include/mosquitto.h"
#include "include/utils.h"
#include "include/device_manager.h"
#include <gpiod.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* For the UUID (found in the TA's h-file(s)) */
#include <main_TA.h>



/* TEE resources */
struct test_ctx {
    TEEC_Context ctx;
    TEEC_Session sess;
};
struct test_ctx ctx;

/* Globals */
char CLEAR[TA_BUFFER_SIZE];
char CIPHER[TA_BUFFER_SIZE];
char TEMP[TA_BUFFER_SIZE];
char MQTT_PUBLISH_TOPIC[TA_BUFFER_SIZE];
int ACTION_CMD_FEATURES[TA_ACTION_COMMAND_FEATURE_SIZE];

/* Mosquitto */
char MQTT_BROKER_IP[] = "test.mosquitto.org"; /* Plug in customized MQTT broker */
int MQTT_BROKER_PORT = 1883;
int MQTT_OPERATION = TA_MQTT_OPERATION_PUBLISH; //default value

/********************************************
 * TA API
 ********************************************/

void prepare_tee_session(struct test_ctx *ctx)
{
    TEEC_UUID uuid = TA_IOT_UUID;
    uint32_t origin;
    TEEC_Result res;

    /* Initialize a context connecting us to the TEE */
    res = TEEC_InitializeContext(NULL, &ctx->ctx);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

    /* Open a session with the TA */
    res = TEEC_OpenSession(&ctx->ctx, &ctx->sess, &uuid,
                           TEEC_LOGIN_PUBLIC, NULL, NULL, &origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
             res, origin);
}

void terminate_tee_session(struct test_ctx *ctx)
{
    TEEC_CloseSession(&ctx->sess);
    TEEC_FinalizeContext(&ctx->ctx);
}

/*
 * Process command TA_CMD_AES_GCM_PREPARE. API in main_TA.h
 */
void prepare_aes(struct test_ctx *ctx, int aes_code, int algo, int is_key_loaded)
{
    // if key/iv are provided from NW, then is_key_loaded = 0 should be passed.
    TEEC_Operation op;
    uint32_t origin;
    TEEC_Result res;

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
                                     TEEC_VALUE_INPUT,
                                     TEEC_VALUE_INPUT,
                                     TEEC_VALUE_INPUT);

    op.params[0].value.a = algo;
    op.params[1].value.a = TA_AES_SIZE_128BIT;
    op.params[2].value.a = aes_code;
    op.params[3].value.a = is_key_loaded;

    res = TEEC_InvokeCommand(&ctx->sess, TA_CMD_AES_GCM_PREPARE,
                             &op, &origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand(PREPARE) failed 0x%x origin 0x%x",
             res, origin);
}

/*
 * Process command TA_CMD_AES_GCM_ENCRYPT. API in main_TA.h
 */
void encrypt_buffer(struct test_ctx *ctx, char *in, size_t in_sz, char *out, size_t out_sz)
{
    TEEC_Operation op;
    uint32_t origin;
    TEEC_Result res;

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_NONE, TEEC_NONE);
    op.params[0].tmpref.buffer = in;
    op.params[0].tmpref.size = in_sz;
    op.params[1].tmpref.buffer = out;
    op.params[1].tmpref.size = out_sz;

    res = TEEC_InvokeCommand(&ctx->sess, TA_CMD_AES_GCM_ENCRYPT,
                             &op, &origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand(encrypt_buffer) failed 0x%x origin 0x%x",
             res, origin);
}

/*
 * Process command TA_CMD_AES_GCM_DECRYPT. API in main_TA.h
 */
void decrypt_buffer(struct test_ctx *ctx, char *in, size_t in_sz, char *out, size_t out_sz)
{
    TEEC_Operation op;
    uint32_t origin;
    TEEC_Result res;

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_NONE, TEEC_NONE);
    op.params[0].tmpref.buffer = in;
    op.params[0].tmpref.size = in_sz;
    op.params[1].tmpref.buffer = out;
    op.params[1].tmpref.size = out_sz;

    res = TEEC_InvokeCommand(&ctx->sess, TA_CMD_AES_GCM_DECRYPT,
                             &op, &origin);
    if (res != TEEC_SUCCESS)
        errx(1, "TEEC_InvokeCommand(decrypt_buffer) failed 0x%x origin 0x%x",
             res, origin);
}

/*
 * Process command TA_CMD_SETUP_DEVICE_INFO. API in main_TA.h
 */
int TA_setup_device_info(struct test_ctx *ctx, char *in, size_t in_sz)
{
    TEEC_Operation op;
    uint32_t origin;
    TEEC_Result res;

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_NONE,
                                     TEEC_NONE, TEEC_NONE);
    op.params[0].tmpref.buffer = in;
    op.params[0].tmpref.size = in_sz;

    res = TEEC_InvokeCommand(&ctx->sess, TA_CMD_SETUP_DEVICE_INFO,
                             &op, &origin);
    if (res != TEEC_SUCCESS){
        errx(1, "TEEC_InvokeCommand(SETUP_DEVICE_INFO) failed 0x%x origin 0x%x",
             res, origin);
        return 0;
    }
    return 1;
}

/*
 * Process command TA_CMD_NUM_SUBSCRIBER_DEVICES. API in main_TA.h
 */
int TA_get_num_subscriber_devices(struct test_ctx *ctx, int *out){
    TEEC_Operation op;
    uint32_t origin;
    TEEC_Result res;

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT,
                                     TEEC_NONE,
                                     TEEC_NONE, TEEC_NONE);

    res = TEEC_InvokeCommand(&ctx->sess, TA_CMD_NUM_SUBSCRIBER_DEVICES,
                             &op, &origin);
    if (res != TEEC_SUCCESS){
        errx(1, "TEEC_InvokeCommand(TA_CMD_NUM_SUBSCRIBER_DEVICES) failed 0x%x origin 0x%x",
             res, origin);
        return 0;
    }

    if (out) *out = op.params[0].value.a;

    return 1;
}

/*
 * Process command TA_CMD_SUBSCRIBER_TOPICS. API in main_TA.h
 */
int TA_get_subscriber_topics(struct test_ctx *ctx, int index, char *out, size_t out_sz)
{
    TEEC_Operation op;
    uint32_t origin;
    TEEC_Result res;

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_NONE, TEEC_NONE);
    op.params[0].value.a = index;
    op.params[1].tmpref.buffer = out;
    op.params[1].tmpref.size = out_sz;

    res = TEEC_InvokeCommand(&ctx->sess, TA_CMD_SUBSCRIBER_TOPICS,
                             &op, &origin);
    if (res != TEEC_SUCCESS){
        errx(1, "TEEC_InvokeCommand(TA_CMD_SUBSCRIBER_TOPICS) failed 0x%x origin 0x%x",
             res, origin);
        return 0;
    }
    return 1;
}

/*
 * Process command TA_CMD_ENCRYPT_TRIGGER_EVENT. API in main_TA.h
 */
int TA_encrypt_trigger_event(struct test_ctx *ctx, char *in, size_t in_sz, char *out, size_t out_sz, char *topic, size_t topic_sz, int device_type, int mqtt_op_type)
{
    TEEC_Operation op;
    uint32_t origin;
    TEEC_Result res;

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_VALUE_INPUT);
    op.params[0].tmpref.buffer = in;
    op.params[0].tmpref.size = in_sz;
    op.params[1].tmpref.buffer = out;
    op.params[1].tmpref.size = out_sz;
    op.params[2].tmpref.buffer = topic;
    op.params[2].tmpref.size = topic_sz;
    op.params[3].value.a = device_type;
    op.params[3].value.b = mqtt_op_type;

    res = TEEC_InvokeCommand(&ctx->sess, TA_CMD_ENCRYPT_TRIGGER_EVENT,
                             &op, &origin);
    if (res != TEEC_SUCCESS){
        errx(1, "TEEC_InvokeCommand(ENCRYPT_TRIGGER_EVENT) failed 0x%x origin 0x%x",
             res, origin);
        return 0;
    }

    return 1;
}

/*
 * Process command TA_CMD_DECRYPT_ACTION_EVENT. API in main_TA.h
 */
int TA_decrypt_action_event(struct test_ctx *ctx, char *in, size_t in_sz, int *features, size_t feature_size, char *arg, size_t arg_sz)
{
    TEEC_Operation op;
    uint32_t origin;
    TEEC_Result res;

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE);
    op.params[0].tmpref.buffer = in;
    op.params[0].tmpref.size = in_sz;
    op.params[1].tmpref.buffer = features;
    op.params[1].tmpref.size = feature_size;
    op.params[2].tmpref.buffer = arg;
    op.params[2].tmpref.size = arg_sz;

    res = TEEC_InvokeCommand(&ctx->sess, TA_CMD_DECRYPT_ACTION_EVENT,
                             &op, &origin);

    if (res == TEEC_SUCCESS){
        printf("device_type=%d, command_type=%d, argument_type=%d\n", features[0],features[1],features[2]);
        if (features[3] > 0) printf("argument: %s. size=%d\n",arg, features[3]);
        return 1;
    }
    else if (res == TEEC_ERROR_GENERIC){
        return 0;
    }
    else{
        errx(1, "TEEC_InvokeCommand(DECRYPT_ACTION_EVENT) failed 0x%x origin 0x%x",
             res, origin);
    }
    return 0;
}


/********************************************
 * AES GCM
 ********************************************/

/*
 * Testing encryption and decryption in SW.
 */
void test_aes_gcm(){
    memset(CLEAR, 0, TA_BUFFER_SIZE); /* clear the array */
    memset(CIPHER, 0, TA_BUFFER_SIZE); /* clear the array */
    memset(TEMP, 0, TA_BUFFER_SIZE); /* clear the array */

    char clear_text_sample[] = "Testing AES GCM Cipher with TZ";

    memcpy(CLEAR, clear_text_sample, strlen(clear_text_sample));
    CLEAR[strlen(clear_text_sample)] = '\0';

    printf("Prepare encode operation\n");
    prepare_aes(&ctx, TA_AES_MODE_ENCODE, TA_AES_ALGO_GCM, 1);

    printf("Encode buffer from TA\n");
    encrypt_buffer(&ctx, CLEAR, TA_BUFFER_SIZE, CIPHER, TA_BUFFER_SIZE + TA_AES_GCM_TAG_SIZE);

    //printf("=> clear = %s\n", CLEAR);
    //printf("=> ciph = %s\n", CIPHER);

    printf("Prepare decode operation\n");
    prepare_aes(&ctx, TA_AES_MODE_DECODE, TA_AES_ALGO_GCM, 1);

    printf("Decode buffer from TA\n");
    decrypt_buffer(&ctx, CIPHER, TA_BUFFER_SIZE + TA_AES_GCM_TAG_SIZE, TEMP, TA_BUFFER_SIZE);
    printf("=> temp = %s\n", TEMP);

    /* Check decoded is the clear content */
    if (memcmp(CLEAR, TEMP, TA_BUFFER_SIZE))
        printf("Clear text and decoded text differ => ERROR\n");
    else
        printf("Clear text and decoded text match\n");
}


/********************************************
 * Publish/Subscribe Methods
 ********************************************/

/*
 * Received Action Command from Server: encrypted action command is decrypted in SW, and the relevant command features are parsed to actuate the device.
 * @params: received encrypted payload, payload length.
 */
void received_action_command(char* payload, int payloadLen){
    memset(CLEAR, 0, TA_BUFFER_SIZE); /* clear the array */
    memset(ACTION_CMD_FEATURES, 0, TA_ACTION_COMMAND_FEATURE_SIZE); /* clear the array */
    prepare_aes(&ctx, TA_AES_MODE_DECODE, TA_AES_ALGO_GCM, 1);

    if(TA_decrypt_action_event(&ctx, payload, payloadLen, ACTION_CMD_FEATURES, TA_ACTION_COMMAND_FEATURE_SIZE, CLEAR, TA_BUFFER_SIZE)){
        int device_type = ACTION_CMD_FEATURES[0], command_type = ACTION_CMD_FEATURES[1], argument_type = ACTION_CMD_FEATURES[2];
        invoke_actuator(device_type, command_type, argument_type, CLEAR);
    }
}

/*
 * Publish Trigger Event: Build and encrypt the event with device's value/readings in SW; publish event via MQTT after retrieving topic name from device's info stored in SW.
 * @params: type of the device (macro), device value/sensor reading
 */
void publish_sensor_data(struct mosquitto *mosq, int device_type, char *buffer)
{
    //printf("buffer: %s\n", buffer);
    memset(CIPHER, 0x0, sizeof(CIPHER));
    memset(MQTT_PUBLISH_TOPIC, 0x0, sizeof(MQTT_PUBLISH_TOPIC));
    prepare_aes(&ctx, TA_AES_MODE_ENCODE, TA_AES_ALGO_GCM, 1);

    if (TA_encrypt_trigger_event(&ctx, buffer, strlen(buffer), CIPHER, TA_BUFFER_SIZE+TA_AES_GCM_TAG_SIZE, MQTT_PUBLISH_TOPIC, TA_BUFFER_SIZE, device_type, TA_MQTT_OPERATION_PUBLISH)){
        printf("Publishing to %s\n", MQTT_PUBLISH_TOPIC);
        int rc = mosquitto_publish(mosq, NULL, MQTT_PUBLISH_TOPIC, TA_BUFFER_SIZE+TA_AES_GCM_TAG_SIZE, CIPHER, 2, false);
        if(rc != MOSQ_ERR_SUCCESS){
            fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
        }
    }
}

/****************************************************
* Trigger functions: add IoT specific triggers here
*****************************************************/

/*
 * Simulate temperature sensor reading.
 */
int get_simulated_temperature_sensor_data(struct mosquitto *mosq){
    sleep(1);
    char payload[20];
    int temperature;

    int count = 0, limit=1; // for testing purpose only
    while (count < limit){
        temperature = random()%100; /* Get random data */
        snprintf(payload, sizeof(payload), "%d", temperature);
        publish_sensor_data(mosq, TA_DEVICE_TYPE_SENSOR_TEMPERATURE, payload);
        count += 1;
        sleep(1); /* Prevent a storm of messages - this pretend sensor works at 1Hz */
    }
    return 0;
}

/*
 * Simulate motion sensor reading.
 */
int get_simulated_motion_sensor_data(struct mosquitto *mosq){
    sleep(1);
    char payload[20];
    int m;

    int count = 0, limit=1; // for testing purpose only
    while (count < limit){
        m = 1; /* Get random data */
        snprintf(payload, sizeof(payload), "%d", m);
        publish_sensor_data(mosq, TA_DEVICE_TYPE_SENSOR_MOTION, payload);
        count += 1;
        sleep(1); /* Prevent a storm of messages - this pretend sensor works at 1Hz */
    }
    return 0;
}

/*
 * A Motion Sensor with RPi3 that checks for motion in every 1 second and fires Trigger event upon detection of motion.
 * @devices: Raspberry Pi 3, PIR Motion Sensor
 */
int get_rpi3_motion_sensor_data(struct mosquitto *mosq){
    char *chipname = "gpiochip0";
    char sensor[] = "MotionSensor";
    unsigned int line_num = 17;	// GPIO Pin #
    unsigned int val;
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    int i, ret;

    chip = gpiod_chip_open_by_name(chipname);
    if (!chip) {
        perror("Open chip failed\n");
        goto end;
    }
    line = gpiod_chip_get_line(chip, line_num);
    if (!line) {
        perror("Get line failed\n");
        goto close_chip;
    }
    ret = gpiod_line_request_input(line, sensor);
    if (ret < 0) {
        perror("Request line as input failed\n");
        goto release_line;
    }

    int count = 0, limit=10; // for testing purpose only
    while (count < limit){
        val = gpiod_line_get_value(line);
        if (val < 0) {
            perror("Read line input failed\n");
            goto release_line;
        }
        if (val == 1){
            printf("Output %u on line #%u\n", val, line_num);
            publish_sensor_data(mosq, TA_DEVICE_TYPE_SENSOR_MOTION, "1");
            count += 1;
            val = 0;
        }
        sleep(1); /* For not overwhelming the device */
    }

    release_line:
    gpiod_line_release(line);
    close_chip:
    gpiod_chip_close(chip);
    end:
    return 0;
}


/********************************************
 * Mosquitto
 ********************************************/

/* Callback called when the client receives a CONNACK message from the broker. */
void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
    /* Print out the connection result. mosquitto_connack_string() produces an
     * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
     * clients is mosquitto_reason_string().
     */
    printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
    if(reason_code != 0){
        /* If the connection fails for any reason, we don't want to keep on
         * retrying in this example, so disconnect. Without this, the client
         * will attempt to reconnect. */
        mosquitto_disconnect(mosq);
        return;
    }

    /* You may wish to set a flag here to indicate to your application that the
     * client is now connected. */

    int num_sub_devices = 0;
    TA_get_num_subscriber_devices(&ctx, &num_sub_devices);
    printf("# of subscriber devices=%d\n", num_sub_devices);

    if (num_sub_devices == 0){
        fprintf(stderr, "Error! Couldn't find topic names to subscribe!\n");
        mosquitto_disconnect(mosq);
        return;
    }

    char** topicList = (char**)malloc(num_sub_devices * sizeof(char*));
    for (int i = 0; i < num_sub_devices; ++i) {
        memset(CLEAR, 0, TA_BUFFER_SIZE); /* clear the array */
        if(TA_get_subscriber_topics(&ctx, i, CLEAR, TA_BUFFER_SIZE)){
            char *topic = (char*)malloc(strlen(CLEAR)+1);
            memcpy(topic, CLEAR, strlen(CLEAR));
            topic[strlen(CLEAR)] = '\0';
            topicList[i] = topic;
            printf("Topic#%d = %s\n", i, topicList[i]);
        }
    }

    //int rc = mosquitto_subscribe(mosq, NULL, "mqttSubTopic", 1);
    int rc = mosquitto_subscribe_multiple(mosq, NULL, num_sub_devices, topicList, 1, 0, NULL);
    if(rc != MOSQ_ERR_SUCCESS){
        fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
        /* We might as well disconnect if we were unable to subscribe */
        mosquitto_disconnect(mosq);
    }

    /* cleanup */
    for(int i=0; i<num_sub_devices; i++ ) {
        free(topicList[i]);
    }
    free(topicList);
}

/* Callback called when the broker sends a SUBACK in response to a SUBSCRIBE. */
void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
    int i;
    bool have_subscription = false;

    /* In this example we only subscribe to a single topic at once, but a
     * SUBSCRIBE can contain many topics at once, so this is one way to check
     * them all. */
    for(i=0; i<qos_count; i++){
        printf("on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
        if(granted_qos[i] <= 2){
            have_subscription = true;
        }
    }
    if(have_subscription == false){
        /* The broker rejected all of our subscriptions, we know we only sent
         * the one SUBSCRIBE, so there is no point remaining connected. */
        fprintf(stderr, "Error: All subscriptions rejected.\n");
        mosquitto_disconnect(mosq);
    }
}

/* Callback called when the client receives a message. */
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    /* This blindly prints the payload, but the payload can be anything so take care. */
    printf("Received Message from: %s, qos=%d, length=%d\n", msg->topic, msg->qos, msg->payloadlen);
    char* payload = (char*) msg->payload;

    int payloadlen = msg->payloadlen;
    if (payloadlen > 0){
        received_action_command(payload, payloadlen);
    }
}

/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
void on_publish(struct mosquitto *mosq, void *obj, int mid)
{
    printf("Message with mid %d has been published.\n", mid);
}


/*
 * Initialize and start the mosquitto services.
 */
int start_mosquitto(){
    printf("*** Starting MQTT ***\n");
    struct mosquitto *mosq;
    int rc;

    /* Required before calling other mosquitto functions */
    mosquitto_lib_init();

    /* Create a new client instance.
     * id = NULL -> ask the broker to generate a client id for us
     * clean session = true -> the broker should remove old sessions when we connect
     * obj = NULL -> we aren't passing any of our private data for callbacks
     */
    mosq = mosquitto_new(NULL, true, NULL);
    if(mosq == NULL){
        fprintf(stderr, "Error: Out of memory.\n");
        return 1;
    }

    /* Configure callbacks. This should be done before connecting ideally. */
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_subscribe_callback_set(mosq, on_subscribe);
    mosquitto_publish_callback_set(mosq, on_publish);
    mosquitto_message_callback_set(mosq, on_message);

    /* Connect to test.mosquitto.org on port 1883, with a keepalive of 60 seconds.
     * This call makes the socket connection only, it does not complete the MQTT
     * CONNECT/CONNACK flow, you should use mosquitto_loop_start() or
     * mosquitto_loop_forever() for processing net traffic. */
    rc = mosquitto_connect(mosq, MQTT_BROKER_IP, MQTT_BROKER_PORT, 60);
    if(rc != MOSQ_ERR_SUCCESS){
        mosquitto_destroy(mosq);
        fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
        return 1;
    }

    /* Run the network loop in a background thread, this call returns quickly. */
    rc = mosquitto_loop_start(mosq);
    if(rc != MOSQ_ERR_SUCCESS){
        mosquitto_destroy(mosq);
        fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
        return 1;
    }

    /* At this point the client is connected to the network socket, but may not
     * have completed CONNECT/CONNACK.
     * It is fairly safe to start queuing messages at this point, but if you
     * want to be really sure you should wait until after a successful call to
     * the connect callback.
     * In this case we know it is 1 second before we start publishing.
     */
    sleep(1);

    if(MQTT_OPERATION == TA_MQTT_OPERATION_PUBLISH){
        printf("Starting MQTT Publisher...\n");
        /* TODO: Plug in code for fetching and publishing real sensor data */
        //get_rpi3_motion_sensor_data(mosq);
        get_simulated_motion_sensor_data(mosq);
        //get_simulated_temperature_sensor_data(mosq);
    }else if(MQTT_OPERATION == TA_MQTT_OPERATION_SUBSCRIBE){
        printf("Starting MQTT Subscriber...\n");
        mosquitto_loop_forever(mosq, -1, 1); /* For subscriber */
    }else{
        printf("Starting MQTT Publisher & Subscriber...\n");
        mosquitto_loop_start(mosq);
        sleep(1);
        get_simulated_temperature_sensor_data(mosq);
        sleep(1);
        get_simulated_motion_sensor_data(mosq);
    }
    mosquitto_lib_cleanup();
}

/*
 * Setup basic device information in SW. Device information is stored in a json file. The json file name is taken from command line as argv[1].
 */
int setup_device_info(char *filename){
    /* open the file */
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error: Unable to open the file: %s\n", filename);
        return 0;
    }

    /* read the file contents into a string */
    char buffer[2048];
    int len = fread(buffer, 1, sizeof(buffer), fp);
    fclose(fp);

    return TA_setup_device_info(&ctx, buffer, strlen(buffer));
}


/********************************************
 * MAIN
 ********************************************/

int main(int argc, char *argv[])
{
    if(argc < 2){
        fprintf(stderr, "\nPlease enter valid arguments to run the program..\n "
                        "Usage: %s [device_info_json_filepath] \n "
                        "Optional arguments: -op [MQTT Operation: Publish=0 (default), Subscribe=1]\n "
                        "Example: %s sample_device_info.json -op 1\n", argv[0], argv[0]);
        printf("Enter a character before exit ...\n");
        getchar();
        return -1;
    }

    /* Read arguments */
    char *device_info_filename = argv[1];
    printf("device_info_filename: %s\n", device_info_filename);

    /* Read optional arguments */
    MQTT_OPERATION = find_int_arg(argc, argv, "-op", TA_MQTT_OPERATION_PUBLISH);

    /* Initialize TEE */
    printf("*** Prepare session with the TA ***\n");
    prepare_tee_session(&ctx);

    /* Test AES GCM */
    //test_aes_gcm();

    /* Setup IoT devices information in the SW */
    if (setup_device_info(device_info_filename) == 0){
        printf("Device setup failed! Please try with valid device information file...\n");
        terminate_tee_session(&ctx);
        return 0;
    }

    /* Start MQTT Publish/Subscribe method. */
    start_mosquitto();

    /* Cleanup */
    terminate_tee_session(&ctx);
	return 0;
}
