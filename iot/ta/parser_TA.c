#include "parser_TA.h"
#include <cJSON_TA.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils_TA.h"
#include <main_TA.h>
#include <tee_internal_api.h>


int parse_device_info(char *info, network *net){
    int status = 0;

    // parse the JSON data
    cJSON *json = cJSON_Parse(info);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) printf("Error: %s\n", error_ptr);
        goto end;
    }

    /* user ID */
    cJSON *uid = cJSON_GetObjectItemCaseSensitive(json, "userID");
    if (!cJSON_IsString(uid) || (uid->valuestring == NULL)){
        printf("Parser:: couldn't parse userID!\n");
        goto end;
    }
    //printf("Checking uid: \"%s\"\n", uid->valuestring);
    net->userID = malloc((strlen(uid->valuestring)+1)* sizeof(char));
    memcpy(net->userID, uid->valuestring, strlen(uid->valuestring));
    net->userID[strlen(uid->valuestring)] = '\0';

    /* IoT devices */
    const cJSON *device = NULL;
    const cJSON *devices = NULL;
    devices = cJSON_GetObjectItemCaseSensitive(json, "devices");
    if (!cJSON_IsArray(devices) || cJSON_IsNull(devices)){
        printf("Parser::Error! Not an array!\n");
        goto end;
    }

    int num_devices = cJSON_GetArraySize(devices);
    net->n = num_devices;
    net->devices = malloc(net->n * sizeof(iot));

    int count = 0;
    cJSON_ArrayForEach(device, devices){
        iot iot_device = {0};

        /* device ID */
        cJSON *did = cJSON_GetObjectItemCaseSensitive(device, "deviceID");
        if (!cJSON_IsString(did) || (did->valuestring == NULL)){
            printf("Parser:: couldn't parse deviceID!");
            goto end;
        }
        //printf("Checking did: \"%s\"\n", did->valuestring);
        iot_device.deviceID = malloc((strlen(did->valuestring)+1)* sizeof(char));
        memcpy(iot_device.deviceID, did->valuestring, strlen(did->valuestring));
        iot_device.deviceID[strlen(did->valuestring)] = '\0';

        /* capability */
        cJSON *cap = cJSON_GetObjectItemCaseSensitive(device, "capability");
        if (!cJSON_IsString(cap) || (cap->valuestring == NULL)){
            printf("Parser:: couldn't parse capability!");
            goto end;
        }
        //printf("Checking cap: \"%s\"\n", cap->valuestring);
        iot_device.capability = malloc((strlen(cap->valuestring)+1)* sizeof(char));
        memcpy(iot_device.capability, cap->valuestring, strlen(cap->valuestring));
        iot_device.capability[strlen(cap->valuestring)] = '\0';

        /* attribute */
        cJSON *attr = cJSON_GetObjectItemCaseSensitive(device, "attribute");
        if (!cJSON_IsString(attr) || (attr->valuestring == NULL)){
            printf("Parser:: couldn't parse attribute!");
            goto end;
        }
        //printf("Checking attr: \"%s\"\n", attr->valuestring);
        iot_device.attribute = malloc((strlen(attr->valuestring)+1)* sizeof(char));
        memcpy(iot_device.attribute, attr->valuestring, strlen(attr->valuestring));
        iot_device.attribute[strlen(attr->valuestring)] = '\0';

        /* unit */
        cJSON *unit = cJSON_GetObjectItemCaseSensitive(device, "unit");
        if (!cJSON_IsString(unit) || (unit->valuestring == NULL)){
            printf("Parser:: couldn't parse unit!");
            goto end;
        }
        //printf("Checking unit: \"%s\"\n", unit->valuestring);
        iot_device.unit = malloc((strlen(unit->valuestring)+1)* sizeof(char));
        memcpy(iot_device.unit, unit->valuestring, strlen(unit->valuestring));
        iot_device.unit[strlen(unit->valuestring)] = '\0';

        /* mqtt topic */
        cJSON *topic = cJSON_GetObjectItemCaseSensitive(device, "topic");
        if (!cJSON_IsString(topic) || (topic->valuestring == NULL)){
            printf("Parser:: couldn't parse topic!");
            goto end;
        }
        //printf("Checking topic: \"%s\"\n", topic->valuestring);
        iot_device.mqtt_topic = malloc((strlen(topic->valuestring)+1)* sizeof(char));
        memcpy(iot_device.mqtt_topic, topic->valuestring, strlen(topic->valuestring));
        iot_device.mqtt_topic[strlen(topic->valuestring)] = '\0';

        /* mqtt pub/sub type */
        cJSON *mqtt_op = cJSON_GetObjectItemCaseSensitive(device, "pub/sub");
        if (!cJSON_IsString(mqtt_op) || (mqtt_op->valuestring == NULL)){
            printf("Parser:: couldn't parse pub/sub!");
            goto end;
        }
        //printf("Checking mqtt_op: \"%s\"\n", mqtt_op->valuestring);
        iot_device.mqtt_op_type = (strcmp(toLower(mqtt_op->valuestring), "publish") == 0 || strcmp(toLower(mqtt_op->valuestring), "pub") == 0) ? TA_MQTT_OPERATION_PUBLISH : TA_MQTT_OPERATION_SUBSCRIBE;

        /* TODO: add similar parsing for other device properties */

        net->devices[count] = iot_device;
        count++;
    }
    status = 1; // successful parsing

    end:
    cJSON_Delete(json);

    return status;
}


int parse_action_command(char *action, iot *event_properties, char **device_command, char **additional_arg, char **additional_arg_value, long *timestamp){
    int status = 0;
    cJSON *command_json = cJSON_Parse(action);

    if (command_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) printf("Parser:: Error before: %s\n", error_ptr);
        goto end;
    }

    /* device ID */
    const cJSON *deviceList = cJSON_GetObjectItem(command_json, ACTION_EVENT_KEY_DEVICES);
    if (!cJSON_IsArray(deviceList) || cJSON_IsNull(deviceList)){
        printf("Parser:: couldn't parse devices!");
        goto end;
    }
    for (int i = 0 ; i < cJSON_GetArraySize(deviceList) ; i++){
        char *did = cJSON_GetArrayItem(deviceList, i)->valuestring;
        //printf("Device id: %s\n", did);
        event_properties->deviceID = TEE_Malloc(strlen(did), 0);
        TEE_MemMove(event_properties->deviceID, did, strlen(did));
        break;
    }

    /* command list */
    const cJSON *obj = NULL;
    const cJSON *commandList = cJSON_GetObjectItem(command_json, ACTION_EVENT_KEY_COMMANDS);
    if (!cJSON_IsArray(commandList) || cJSON_IsNull(commandList)){
        printf("Parser:: couldn't parse commandList!");
        goto end;
    }
    for (int i = 0 ; i < cJSON_GetArraySize(commandList) ; i++){
        obj = cJSON_GetArrayItem(commandList, i);
        break;
    }

    /* capability */
    const cJSON *capability = cJSON_GetObjectItem(obj, ACTION_EVENT_KEY_CAPABILITY);
    if (!cJSON_IsString(capability) || (capability->valuestring == NULL)){
        printf("Parser:: couldn't parse capability!");
        goto end;
    }
    //printf("capability: %s\n", capability->valuestring);
    event_properties->capability = TEE_Malloc(strlen(capability->valuestring), 0);
    TEE_MemMove(event_properties->capability, capability->valuestring, strlen(capability->valuestring));

    /* command */
    const cJSON *cmd = cJSON_GetObjectItem(obj, ACTION_EVENT_KEY_COMMAND);
    if (!cJSON_IsString(cmd) || (cmd->valuestring == NULL)){
        printf("Parser:: couldn't parse cmd!");
        goto end;
    }
    //printf("cmd: %s\n", cmd->valuestring);
    *device_command = TEE_Malloc(strlen(cmd->valuestring), 0);
    TEE_MemMove(*device_command, cmd->valuestring, strlen(cmd->valuestring));

    /* arguments */
    *additional_arg = NULL;
    *additional_arg_value = NULL;
    const cJSON *argumentList = cJSON_GetObjectItem(obj, ACTION_EVENT_KEY_ARGUMENTS);
    if (!cJSON_IsArray(argumentList) || cJSON_IsNull(argumentList)){
        printf("Parser:: couldn't parse arguments!");
        goto end;
    }
    for (int i = 0 ; i < cJSON_GetArraySize(argumentList) ; i++){
        const cJSON *argObj = cJSON_GetArrayItem(argumentList, i)->child;
        //printf("arg key: %s, value:%s | %d, %d\n", argObj->string, argObj->valuestring, argObj->type, argObj->valueint);

        *additional_arg = TEE_Malloc(strlen(argObj->string), 0);
        TEE_MemMove(*additional_arg, argObj->string, strlen(argObj->string));

        *additional_arg_value = TEE_Malloc(strlen(argObj->valuestring), 0);
        TEE_MemMove(*additional_arg_value, argObj->valuestring, strlen(argObj->valuestring));
        break;
    }

    /* timestamp */
    const cJSON *ts = cJSON_GetObjectItem(command_json, EVENT_KEY_TIMESTAMP);
    if (!cJSON_IsString(ts) || (ts->valuestring == NULL)){
        printf("Parser:: couldn't parse timestamp!");
        goto end;
    }
    *timestamp = (long) atod_ta(ts->valuestring);


    status = 1; // successful parsing

    end:
    cJSON_Delete(command_json);
    return status;
}

int build_trigger_event(char **event_str, iot *device_prop, long ts){
    cJSON *event = cJSON_CreateObject();
    cJSON *capability = cJSON_CreateObject();
    cJSON *attribute = cJSON_CreateObject();
    char *string = NULL;
    int status = 0;

    /* device ID */
    if (cJSON_AddStringToObject(event, EVENT_KEY_DEVICEID, device_prop->deviceID) == NULL){
        printf("Failed to add device ID.\n");
        goto end;
    }

    /* value */
    cJSON *value = NULL;
    value = device_prop->value_type == STRING ? cJSON_CreateString(device_prop->value) : cJSON_CreateNumber(atod_ta(device_prop->value));
    if (value == NULL){
        printf("Failed to add value.\n");
        goto end;
    }
    cJSON_AddItemToObject(attribute, EVENT_KEY_VALUE, value);

    /* unit */
    cJSON *unit = NULL;
    unit = cJSON_CreateString(device_prop->unit);
    if (unit == NULL){
        printf("Failed to add unit.\n");
        goto end;
    }
    cJSON_AddItemToObject(attribute, EVENT_KEY_UNIT, unit);

    /* timestamp */
    char ts_buffer[16];
    snprintf(ts_buffer,16, "%ld", ts);
    cJSON *timestamp = cJSON_CreateString(ts_buffer);
    cJSON_AddItemToObject(attribute, EVENT_KEY_TIMESTAMP, timestamp);

    cJSON_AddItemToObject(capability, device_prop->attribute, attribute);
    cJSON_AddItemToObject(event, device_prop->capability, capability);

    /* json to string */
    string = cJSON_Print(event);
    if (string == NULL)
    {
        printf("Failed cJSON_Print.\n");
        goto end;
    }

    TEE_MemMove(*event_str, string, strlen(string));
    //printf("Parser:: Event = %s\n", *event_str);

    status = 1; // successful json conversion

    end:
    cJSON_Delete(event);
    return status;
}