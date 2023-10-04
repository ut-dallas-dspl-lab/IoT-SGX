/*
 * Copyright (C) 2011-2018 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <mosquitto.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

# include <unistd.h>
# include <pwd.h>
# define MAX_PATH FILENAME_MAX
#define MAX_BUF_LEN 100

#include "sgx_urts.h"
#include "App.h"
#include "Enclave_u.h"
#include "ErrorSupport.h"

#include <sys/uio.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <thread>
#include <vector>
#include <map>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "SocketManager.h"
#include "JSONParser.h"
#include "MongoManager.h"
#include "utils.h"

using namespace std;
using std::chrono::system_clock;
using sec = std::chrono::duration<double>;

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;
sgx_status_t ret = SGX_SUCCESS;
sgx_launch_token_t token = {0};
int updated = 0;

/* Global variables */
bool is_data_obl_enabled = 0;
bool IS_DEBUG = false;
bool is_benchmark = false;
int RULE_COUNT = 0;
int EVENT_COUNT = 0;

char LOG_FILENAME[100];

/* MongoDB */
MongoManager *mongoObj;
char MONGO_URI[] = "mongodb://localhost:27017"; /* Note: Plug in customized db */
char MONGO_DBNAME[] = "IOT"; /* Note: Plug in customized db */

/* Mosquitto */
struct mosquitto *mosq;
char MQTT_BROKER_IP[] = "test.mosquitto.org"; /* Plug in customized MQTT broker */
int MQTT_BROKER_PORT = 1883;


/*
 *   Initialize the enclave: Call sgx_create_enclave to initialize an enclave instance
 */
int initialize_enclave(void)
{
    char token_path[MAX_PATH] = {'\0'};
    sgx_launch_token_t token = {0};
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    int updated = 0;
    
    /* Step 1: try to retrieve the launch token saved by last transaction 
     *         if there is no token, then create a new one.
     */
    /* try to get the token saved in $HOME */
    const char *home_dir = getpwuid(getuid())->pw_dir;
    
    if (home_dir != NULL && 
        (strlen(home_dir)+strlen("/")+sizeof(TOKEN_FILENAME)+1) <= MAX_PATH) {
        /* compose the token path */
        strncpy(token_path, home_dir, strlen(home_dir));
        strncat(token_path, "/", strlen("/"));
        strncat(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME)+1);
    } else {
        /* if token path is too long or $HOME is NULL */
        strncpy(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME));
    }

    FILE *fp = fopen(token_path, "rb");
    if (fp == NULL && (fp = fopen(token_path, "wb")) == NULL) {
        printf("Warning: Failed to create/open the launch token file \"%s\".\n", token_path);
    }

    if (fp != NULL) {
        /* read the token from saved file */
        size_t read_num = fread(token, 1, sizeof(sgx_launch_token_t), fp);
        if (read_num != 0 && read_num != sizeof(sgx_launch_token_t)) {
            /* if token is invalid, clear the buffer */
            memset(&token, 0x0, sizeof(sgx_launch_token_t));
            printf("Warning: Invalid launch token read from \"%s\".\n", token_path);
        }
    }
    /* Step 2: call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        ret_error_support(ret);
        if (fp != NULL) fclose(fp);
        return -1;
    }

    /* Step 3: save the launch token if it is updated */
    if (updated == FALSE || fp == NULL) {
        /* if the token is not updated, or file handler is invalid, do not perform saving */
        if (fp != NULL) fclose(fp);
        return 0;
    }

    /* reopen the file with write capablity */
    fp = freopen(token_path, "wb", fp);
    if (fp == NULL) return 0;
    size_t write_num = fwrite(token, 1, sizeof(sgx_launch_token_t), fp);
    if (write_num != sizeof(sgx_launch_token_t))
        printf("Warning: Failed to save launch token to \"%s\".\n", token_path);
    fclose(fp);

    /* Utilize edger8r attributes */
    edger8r_array_attributes();
    edger8r_pointer_attributes();
    edger8r_type_attributes();
    edger8r_function_attributes();

    /* Utilize trusted libraries */
    ecall_libc_functions();
    ecall_libcxx_functions();
    ecall_thread_functions();

    return 0;
}

/********************************************
 * Publish/Subscribe Methods
 ********************************************/
int publish_response(char *topic, char *payload, int payloadLen){
    printf("#Topic:%s, TextLen=%d\n", topic, payloadLen);
    int rc = mosquitto_publish(mosq, NULL, topic, payloadLen, payload, 2, false);
    if(rc != MOSQ_ERR_SUCCESS){
        fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
        return 0;
    }
    return 1;
}

int received_trigger_events(char *buffer, int buffer_len){
    struct Message *msg = (Message*) malloc( sizeof( struct Message ));
    msg->text = (char *) malloc((buffer_len+1)*sizeof(char));
    msg->textLength = buffer_len;
    msg->tag = NULL;
    msg->timestamp = getCurrentTime(SECOND);

    memcpy(msg->text, buffer, msg->textLength);
    msg->text[msg->textLength] = '\0';
    ecall_did_receive_event_mqtt(global_eid, msg);

    free(msg->text);
    free(msg);
}

/****************************************************
 * MQTT
****************************************************/

/* Callback called when the client receives a CONNACK message from the broker. */
void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
    printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
    if(reason_code != 0){
        mosquitto_disconnect(mosq);
    }

    /* Get number of subscription topics from Enclave */
    int num_subscription = 0;
    ecall_get_num_mqtt_subscription_topic(global_eid, &num_subscription);
    if (num_subscription == 0){
        fprintf(stderr, "Error! No topic names available to subscribe!\n");
        mosquitto_disconnect(mosq);
    }

    /* Get subscription topic names from Enclave */
    struct Message *msg = (Message*) malloc(num_subscription * sizeof( struct Message ));
    struct Message *start_of_msg = msg;

    for(int i=0; i<num_subscription; i++ ) {
        msg->text = (char*)malloc(TA_BUFFER_SIZE+1);
        msg++;
    }
    msg = start_of_msg;

    int ret = 0;
    ecall_get_mqtt_topics(global_eid, &ret, msg, num_subscription);
    if (ret == 0){
        fprintf(stderr, "Error! Couldn't find topic names to subscribe!\n");
        /* cleanup */
        mosquitto_disconnect(mosq);
        msg = start_of_msg;
        for(int i=0; i<num_subscription; i++ ) {
            free(msg->text);
            msg++;
        }
        msg = start_of_msg;
        free(msg);
        return;
    }

    char** topicList = (char**)malloc(num_subscription * sizeof(char*));
    for(int i=0; i<num_subscription; i++) {
        printf("#Topic %s\n", msg->text);
        topicList[i] = msg->text;
        msg++;
    }

    /* mosquitto_subscribe */
    int rc = mosquitto_subscribe_multiple(mosq, NULL, num_subscription, topicList, 1, 0, NULL);
    if(rc != MOSQ_ERR_SUCCESS){
        fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
        /* We might as well disconnect if we were unable to subscribe */
        mosquitto_disconnect(mosq);
    }

    /* cleanup */
    free(topicList);
    msg = start_of_msg;
    for(int i=0; i<num_subscription; i++ ) {
        free(msg->text);
        msg++;
    }
    msg = start_of_msg;
    free(msg);
}

/* Callback called when the broker sends a SUBACK in response to a SUBSCRIBE. */
void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
    int i;
    bool have_subscription = false;
    for(i=0; i<qos_count; i++){
        printf("on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
        if(granted_qos[i] <= 2){
            have_subscription = true;
        }
    }
    if(have_subscription == false){
        fprintf(stderr, "Error: All subscriptions rejected.\n");
        mosquitto_disconnect(mosq);
    }
}

/* Callback called when the client receives a message. */
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msgMQTT)
{
    printf("\n***********************\nNew Message\n***********************\n");
    printf("%s %d %d, %d\n", msgMQTT->topic, msgMQTT->qos, msgMQTT->payloadlen, strlen(msgMQTT->topic));
    char* buffer = (char *)msgMQTT->payload;
    if (strcmp(buffer, "quit") == 0){
        printf("Disconnecting MQTT...\n");
        mosquitto_disconnect(mosq);
        return;
    }

    if (msgMQTT->payloadlen > 0) received_trigger_events(buffer, msgMQTT->payloadlen);
}

void on_publish(struct mosquitto *mosq, void *obj, int mid)
{
    printf("Message with mid %d has been published from Subscriber.\n", mid);
}


/****************************************************
 * OCall functions
 ****************************************************/

void ocall_print_string(const char *str)
{
    /* Proxy/Bridge will check the length and null-terminate 
     * the input string to prevent buffer overflow. 
     */
    printf("%s\n", str);
}

size_t ocall_write_to_file(DatabaseElement *element, int count){
    bool returnValue = mongoObj->insertRule(element, count);
    return returnValue == true? 1:0;
}

size_t ocall_read_rule_count(char *pk, char *sk, size_t queryType){
    if (IS_DEBUG) printf("ocall_read_rule_count\n");
    DatabaseElement *dbElement = (DatabaseElement*) malloc(sizeof(DatabaseElement));
    dbElement->deviceID = pk;
    dbElement->attribute = sk;
    dbElement->queryType = static_cast<DBQueryType>(queryType);
    int ruleCount = mongoObj->retrieveRuleCount(dbElement);
    free(dbElement);
    return ruleCount;
}

size_t ocall_read_rule_info(char *pk, char *sk, size_t queryType, int* rule_size_list, size_t count){
    if (IS_DEBUG) printf("ocall_read_rule_info\n");
    DatabaseElement *dbElement = (DatabaseElement*) malloc(sizeof(DatabaseElement));
    dbElement->deviceID = pk;
    dbElement->attribute = sk;
    dbElement->queryType = static_cast<DBQueryType>(queryType);
    dbElement->data = (Message*) malloc(count * sizeof(struct Message));

    Message *startPos = dbElement->data;
    bool returnValue = mongoObj->retrieveRuleInfo(dbElement, count);
    dbElement->data = startPos;
    if(returnValue){
        for(int i=0; i<count; i++){
            rule_size_list[i] = dbElement->data->textLength;
            dbElement->data++;
        }
    }

    dbElement->data = startPos;
    free(dbElement->data);
    free(dbElement);
    return returnValue == true? 1:0;
}

size_t ocall_read_rule(char *pk, char *sk, size_t queryType, char *data, size_t length, size_t ruleCount){
    if (IS_DEBUG) printf("ocall_read_rule\n");
    DatabaseElement *dbElement = (DatabaseElement*) malloc(sizeof(DatabaseElement));
    dbElement->deviceID = pk;
    dbElement->attribute = sk;
    dbElement->queryType = static_cast<DBQueryType>(queryType);
    dbElement->data = (Message*) malloc(ruleCount * sizeof(Message));;

    Message *startPos = dbElement->data;
    bool returnValue = mongoObj->retrieveRule(dbElement, ruleCount);

    dbElement->data = startPos;
    int total_len = 0;
    if(returnValue){
        for (int i = 0; i < ruleCount; ++i) {
            memcpy(data + total_len, dbElement->data->text, dbElement->data->textLength);
            total_len += dbElement->data->textLength;
            dbElement->data++;
        }
        data[total_len] = '\0';
        if(length != total_len) printf("\nAPP:: Error! length mismatch!\n");
    }

    dbElement->data = startPos;
    free(dbElement->data);
    free(dbElement);

    return returnValue & total_len == length ? 1:0;
}

size_t ocall_send_rule_commands_mqtt(char *topic, char *actionCommand, int commandLength){
    if (IS_DEBUG) printf("---------ocall_send_rule_commands_mqtt----------\n");
    //printf("%s, %d\n", msg->address, msg->textLength);
    return publish_response(topic, actionCommand, commandLength);
}

size_t ocall_get_current_time(int timeUnit){
    return getCurrentTime(timeUnit);
}


/****************************************************
 * MongoDB
****************************************************/
void setup_db(char *db_collection){
    //std::string collectionName = getDatabaseName(0);
    printf("MongoDB Collection: %s\n", db_collection);
    logInfo(LOG_FILENAME, "Database name", db_collection);
    mongoObj = new MongoManager(MONGO_URI, MONGO_DBNAME, db_collection);
    mongoObj->initConnection();
}


/****************************************************
 * Helper functions
 ****************************************************/
void save_statistics(int checkpoint_count, double total_time){
    printf("\n\n ######## Checkpoint:: Count=%d ########\n", checkpoint_count);

    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("Max: %ld  kilobytes\n", usage.ru_maxrss);

    int num_conflict, num_retrieved, num_commands;
    num_conflict = num_retrieved = num_commands = 0;
    ecall_get_stat_num_rule_conflicts(global_eid, &num_conflict);
    ecall_get_stat_num_rule_retrieved(global_eid, &num_retrieved);
    ecall_get_stat_num_device_commands(global_eid, &num_commands);
    printf("TOTAL_CONFLICTED_RULES=%d, TOTAL_RETRIEVED_RULES=%d, TOTAL_DEVICE_COMMANDS=%d\n", num_conflict, num_retrieved, num_commands);

    logCheckpointInfo(LOG_FILENAME, checkpoint_count, num_conflict, num_retrieved, num_commands, usage.ru_maxrss, total_time);
    logMemoryDetails(LOG_FILENAME);
}


/*
 * Setup basic device information in SW. Device information is stored in a json file. Variable deviceInfoFile contains the json file name.
 */
int setup_device_info(char *filename){
    /* open the file */
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error: Unable to open the file: %s.\n", filename);
        return 0;
    }
    logInfo(LOG_FILENAME, "Device info filename", filename);

    /* read the file contents into a string */
    char buffer[4096];
    int len = fread(buffer, 1, sizeof(buffer), fp);
    fclose(fp);

    /* parse string and store info in Enclave */
    int status = 0;
    ecall_setup_device_info(global_eid, &status, buffer);
    return status;
}


int open_socket_for_rules()
{
    printf("\nOpening Socket for Rules...\n");
    char buffer[LIMIT];
    int n;
    SocketManager socketObj(20006);
    int socketConnection = socketObj.establish_connection();

    double total_time = 0;
    clock_t time = clock();
    printf("started the clock for rules...\n");

    while(1){
        bzero(buffer,LIMIT);
        n = read(socketConnection,buffer,LIMIT);
        if (n < 0){
            perror("ERROR! reading from socket\n");
            continue;
        }

        if(strlen(buffer) > 0){
            printf("\n\n***********************\nRule received, #%d\n***********************\n", RULE_COUNT);
            printf("buffer len: %d\n",strlen(buffer));
            if(strcmp(buffer, "quit")==0)
                break;

            const auto start = system_clock::now();

            struct Message *msg = (Message*) malloc( sizeof( struct Message ));
            int returnValue = 0;
            if(parse_data_with_tag(buffer, msg, false)){
                msg->isEncrypted = 1;
                ecall_did_receive_rule(global_eid, &returnValue, msg);
                free(msg->text);
                free(msg->tag);
            }

            free(msg);
            RULE_COUNT++;

            const sec duration = system_clock::now() - start;
            total_time += duration.count();
        }
    }

    printf("Process took %f seconds\n", get_seconds(clock()-time));
    std::cout << "Process took " << total_time << " sec" << std::endl;

    socketObj.close_connection();
    return 0;
}


int start_mqtt_for_trigger_events(){
    printf("\nOpening MQTT for trigger events...\n");
    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if(mosq == NULL){
        fprintf(stderr, "Error: Out of memory.\n");
        return 1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_subscribe_callback_set(mosq, on_subscribe);
    mosquitto_publish_callback_set(mosq, on_publish);
    mosquitto_message_callback_set(mosq, on_message);

    int rc = mosquitto_connect(mosq, MQTT_BROKER_IP, MQTT_BROKER_PORT, 60);
    if(rc != MOSQ_ERR_SUCCESS){
        mosquitto_destroy(mosq);
        fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
        return 1;
    }
    mosquitto_loop_forever(mosq, -1, 1);
    mosquitto_lib_cleanup();
    return 0;
}

/*
 * Application entry
 */
int SGX_CDECL main(int argc, char *argv[])
{
    if(argc < 3){
        fprintf(stderr, "\nPlease enter valid arguments to run the program..\n "
               "Usage: %s [mongoDB collection name] [json filepath containing device information] \n"
               "Optional arguments: -r [Rule: 0/1] -d [IoT: 0/1] -de [debug: 0/1] -bm [benchmark: 0/1]\n"
               "Example: ./app sampledb datafiles/sample_device_info.json -r 1 -d 1 -de 0 -bm 0\n", argv[0]);
        printf("Enter a character before exit ...\n");
        getchar();
        return -1;
    }

    /* Read arguments */
    char *database_collection_name = argv[1];
    char *device_info_filename = argv[2];

    /* Read optional arguments */
    bool is_rule_receiving_enabled = find_bool_arg(argc, argv, "-r", 1); // default true
    bool is_data_receiving_enabled = find_bool_arg(argc, argv, "-d", 1); // default true
    IS_DEBUG = find_bool_arg(argc, argv, "-de", 0);
    is_benchmark = find_bool_arg(argc, argv, "-bm", 0);
    is_data_obl_enabled = find_bool_arg(argc, argv, "-ob", 0);
    printf("Rule operation=%d, IoT operation=%d, Debug: %d, Benchmark\n", is_rule_receiving_enabled, is_data_receiving_enabled, IS_DEBUG, is_benchmark);

    /* Initialize the enclave */
    if(initialize_enclave() < 0){
        printf("Enclave initialization failed!\nEnter a character before exit ...\n");
        getchar();
        return -1; 
    }

    /* Create a log file in "logs" directory */
    get_log_filename(LOG_FILENAME, is_rule_receiving_enabled, is_data_receiving_enabled);
    printf("log filename = %s\n", LOG_FILENAME);

    /* Record initial resource usage statistics */
    struct rusage usage;
    struct timeval startu, endu, starts, ends;
    getrusage(RUSAGE_SELF, &usage);
    startu = usage.ru_utime;
    starts = usage.ru_stime;

    /* Setup the Enclave with argument values */
    ecall_initialize_enclave(global_eid, IS_DEBUG, is_benchmark);

    /* Initialize other services */
    setup_db(database_collection_name);

    /* Setup IoT devices information in the Enclave */
    if (setup_device_info(device_info_filename) == 0){
        printf("Device setup failed! Please try again...\n");
        sgx_destroy_enclave(global_eid);
        return -1;
    }

    int num_threads = 2;
    bool valid_threads[] = {false, false};
    boost::thread t[num_threads];

    /* Start a thread to handle rules */
    if(is_rule_receiving_enabled){
        valid_threads[0] = true;
        t[0] = boost::thread(open_socket_for_rules);
    }

    /* Start a thread to handle IoT device events */
    if(is_data_receiving_enabled){
        valid_threads[1] = true;
        t[1] = boost::thread(start_mqtt_for_trigger_events);
    }

    /* Join all the threads */
    for (int i = 0; i < num_threads; ++i) {
        if(valid_threads[i])  t[i].join();
    }

    /* View/Log benchmark statistics */
    ecall_print_statistics(global_eid);
    save_statistics(-1, -1);

    /* Destroy the enclave */
    sgx_destroy_enclave(global_eid);

    /* Record final resource usage statistics */
    getrusage(RUSAGE_SELF, &usage);
    endu = usage.ru_utime;
    ends = usage.ru_stime;
    printf("user CPU start: %lu.%06u; end: %lu.%06u\n", startu.tv_sec, startu.tv_usec, endu.tv_sec, endu.tv_usec);
    printf("kernel CPU start: %lu.%06u; end: %lu.%06u\n", starts.tv_sec, starts.tv_usec, ends.tv_sec, ends.tv_usec);
    printf("Final Max: %ld  kilobytes\n", usage.ru_maxrss);
    
    printf("Info: Enclave successfully returned.\n");
    printf("Enter a character before exit ...\n");
    getchar();
    return 0;
}
