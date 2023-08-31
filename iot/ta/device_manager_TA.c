#include "device_manager_TA.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <main_TA.h>
#include <tee_internal_api.h>
#include "parser_TA.h"
#include "utils_TA.h"

#include "device_motion_sensor_TA.h"
#include "device_temp_sensor_TA.h"
#include "device_switch_TA.h"


network net; /* Stores user and device information */

/********************************************
 * Helper Functions
 ********************************************/

/*
 * Get device type macro. Macro definition in main_TA.h
 * @params: device capability
 */
int get_device_type(char *capability){
    char *cpb = toLower(capability);

    /* TODO: Plug in other devices */
    if (check_device_type_switch(cpb)) return get_device_type_switch(cpb);
    else if (check_device_type_motion_sensor(cpb)) return TA_DEVICE_TYPE_SENSOR_MOTION;
    else if (check_device_type_temperature_sensor(cpb)) return TA_DEVICE_TYPE_SENSOR_TEMPERATURE;
    return -1;
}

/*
 * Get device information from stored Network of devices according to query properties
 * @params: struct iot with query properties (deviceID, capability, mqtt_op_type), output struct iot pointing to desired device information
 */
int get_device_info(iot *event_info, iot **device_info){
    for (int i = 0; i < net.n; ++i) {
        if (strcmp(net.devices[i].deviceID, event_info->deviceID)==0
        && strcmp(net.devices[i].capability, event_info->capability)==0
        && net.devices[i].mqtt_op_type == TA_MQTT_OPERATION_SUBSCRIBE){
            *device_info = &net.devices[i];
            return 1;
        }
    }
    return 0;
}

/********************************************
 * Device Info
 ********************************************/

/*
 * Parse device information from json string and store in a Global Struct Network
 * @params: json string with device information
 */
int prepare_device_info(char *info){
    int ret = parse_device_info(info, &net);
    if (ret == 0){
        printf("Device info parsing failed!\n");
        return 0;
    }
    /* Add device type macro for each device */
    for (int i = 0; i < net.n; ++i) {
        net.devices[i].device_type = get_device_type(net.devices[i].capability);
        printf("#%d: did=%s, cap=%s, attr=%s, dt=%d, topic=%s\n",i+1, net.devices[i].deviceID, net.devices[i].capability, net.devices[i].attribute, net.devices[i].device_type, net.devices[i].mqtt_topic);
    }
    return 1;
}

/*
 * Return the number of devices acting as a subscriber
 * @params: none
 */
int get_num_subscriber_devices(){
    int num_devices = 0;
    for (int i = 0; i < net.n; ++i) {
        if(net.devices[i].mqtt_op_type == TA_MQTT_OPERATION_SUBSCRIBE) num_devices++;
    }
    return num_devices;
}

/*
 * Get the mqtt topic name for a subscriber device
 * @params: index of the subscriber device, output buffer to store mqtt topic name
 */
int get_subscriber_topic(int sub_topic_index, char** topic_name){
    int num_devices = 0;
    for (int i = 0; i < net.n; ++i) {
        if(net.devices[i].mqtt_op_type == TA_MQTT_OPERATION_SUBSCRIBE) {
            if(num_devices == sub_topic_index){
                TEE_MemMove(*topic_name, net.devices[i].mqtt_topic, strlen(net.devices[i].mqtt_topic));
                return 1;
            }
            num_devices++;
        }
    }
    return 0;
}

void destroy_device_network_info(){
    printf("Freeing resources from devices ...\n");
    for (int i = 0; i < net.n; ++i){
        TEE_Free(net.devices[i].deviceID);
        TEE_Free(net.devices[i].capability);
        TEE_Free(net.devices[i].attribute);
        TEE_Free(net.devices[i].unit);
        TEE_Free(net.devices[i].mqtt_topic);
    }
}

/********************************************
 * Trigger Events
 ********************************************/

/*
 * Prepare trigger event string with sensor reading/value and store in a Global Struct Network
 * @params: sensor reading/value, type of the device (macro), type of mqtt operation (macro), output buffer to store the event string, output buffer to store mqtt topic name
 */
int prepare_trigger_event(char *sensor_value, int device_type, int mqtt_op_type, char **response, char** topic_name){
    iot *device_properties = NULL;
    for (int i = 0; i < net.n; ++i) {
        if (net.devices[i].device_type == device_type && net.devices[i].mqtt_op_type == mqtt_op_type){
            device_properties = &net.devices[i];
            break;
        }
    }

    if (device_properties == NULL){
        printf("ERROR! Missing device properties for the iot device!\n");
        return 0;
    }

    TEE_MemMove(*topic_name, device_properties->mqtt_topic, strlen(device_properties->mqtt_topic));

    /* TODO: Plug in other trigger devices */
    switch (device_type) {
        case TA_DEVICE_TYPE_SENSOR_MOTION:
            return prepare_trigger_for_motion_sensor(sensor_value, response, device_properties);
        case TA_DEVICE_TYPE_SENSOR_TEMPERATURE:
            return prepare_trigger_for_temperature_sensor(sensor_value, response, device_properties);
    }

    return 0;
}


/********************************************
 * Action Commands
 ********************************************/

/*
 * Get actuator command macro. Macro definition in main_TA.h
 * @params: device type (macro), actuator command string
 */
int get_actuator_command(int device_type, char *command){
    char *cmd = toLower(command);

    /* TODO: Plug in other actuator devices */
    switch (device_type) {
        case TA_DEVICE_TYPE_LED:
        case TA_DEVICE_TYPE_LIGHT:
        case TA_DEVICE_TYPE_SWITCH:
            return get_actuator_command_switch(cmd);
    }
    return -1;
}

/*
 * Get actuator argument macro. Macro definition in main_TA.h
 * @params: device type (macro), actuator argument string
 */
int get_actuator_argument(int device_type, char *argument){
    char *arg = toLower(argument);

    /* TODO: Plug in other actuator devices */
    switch (device_type) {
        case TA_DEVICE_TYPE_LED:
        case TA_DEVICE_TYPE_LIGHT:
        case TA_DEVICE_TYPE_SWITCH:
            return get_actuator_argument_switch(arg);
    }
    return -1;
}

/*
 * Parse the action command json string to retrieve actuator commands
 * @params: action command json string, output macro of device type, output macro of command type, output macro of argument type, output buffer to store argument, output buffer size
 */
int prepare_action_command(char *action, int *device_type, int *command_type, int *argument_type, char **argument, int *argument_size){
    iot *event_prop = malloc(1 * sizeof(iot));
    char *device_command = NULL;
    char *additional_arg = NULL;
    char *additional_arg_value = NULL;

    /* Parse action command json */
    int isSuccess = parse_action_command(action, event_prop, &device_command, &additional_arg, &additional_arg_value);
    if(!isSuccess) {
        goto end;
    }
    printf("deviceID=%s, device_capability=%s, device_command=%s\n", event_prop->deviceID, event_prop->capability, device_command);

    /* Fetch stored device specific information */
    iot *device_info = NULL;
    isSuccess = get_device_info(event_prop, &device_info);
    if(!isSuccess) {
        printf("Error! No registered devices found!");
        goto end;
    }

    *device_type = device_info->device_type;
    printf("Device Type = %d\n", *device_type);

    /* Get command type macro */
    *command_type = get_actuator_command(*device_type, device_command);
    if (*command_type < 0){
        printf("Error! Unknown Command Type\n");
        isSuccess = 0;
        goto end;
    }
    printf("Command Type = %d\n", *command_type);

    /* additional arguments */
    if (additional_arg == NULL || additional_arg_value == NULL){
        printf("No additional arguments!");
        goto end;
    }
    *argument_type = get_actuator_argument(*device_type, additional_arg);
    *argument_size = strlen(additional_arg_value);
    TEE_MemMove(*argument, additional_arg_value, strlen(additional_arg_value));


    end:
    TEE_Free(event_prop->deviceID);
    TEE_Free(event_prop->capability);
    TEE_Free(event_prop);
    TEE_Free(device_command);
    TEE_Free(additional_arg);
    TEE_Free(additional_arg_value);

    return isSuccess;
}