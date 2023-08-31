#include "device_switch_TA.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tee_internal_api.h>
#include "device_manager_TA.h"
#include "utils_TA.h"
#include "parser_TA.h"

/********************************************
 * Device Properties
 ********************************************/

int check_device_type_switch(char *capability){
    if (strcmp(capability, DEVICE_CAPABILITY_SWITCH) == 0 || strcmp(capability, DEVICE_CAPABILITY_LIGHT) == 0 || strcmp(capability, DEVICE_CAPABILITY_LED) == 0){
        return 1;
    }
    return 0;
}

int get_device_type_switch(char *capability){
    if (strcmp(capability, DEVICE_CAPABILITY_LED) == 0) return TA_DEVICE_TYPE_LED;
    else if (strcmp(capability, DEVICE_CAPABILITY_SWITCH) == 0) return TA_DEVICE_TYPE_LIGHT;
    else return TA_DEVICE_TYPE_SWITCH;
}

int get_device_value_type_switch(){
    return STRING;
}

/********************************************
 * Trigger
 ********************************************/

int prepare_trigger_for_switch(char *sensor_value, char **response, iot *device_properties){
    memset(TRIGGER_EVENT_BUFFER, 0x0, sizeof(TRIGGER_EVENT_BUFFER));
    char *value = toLower(sensor_value);
    TEE_Time t;
    TEE_GetREETime(&t);

    if(strcmp(value, SWITCH_VALUE_TRUE) == 0 || strcmp(value, SWITCH_VALUE_ON) == 0 || strcmp(value, SWITCH_VALUE_ACTIVE) == 0){
        device_properties->value = SWITCH_VALUE_ON;
    } else if (strcmp(value, SWITCH_VALUE_FALSE) == 0 || strcmp(value, SWITCH_VALUE_OFF) == 0 || strcmp(value, SWITCH_VALUE_INACTIVE) == 0){
        device_properties->value = SWITCH_VALUE_OFF;
    }
    else{
        printf("Error! Unknown switch value!\n");
        return 0;
    }
    device_properties->value_type = get_device_value_type_switch();

    build_trigger_event(response, device_properties, t.seconds);

    //sprintf(TRIGGER_EVENT_BUFFER, TRIGGER_EVENT_TEMPLATE, device_properties->deviceID,  device_properties->capability, device_properties->attribute, device_properties->value, t.seconds);
    //TEE_MemMove((*response), TRIGGER_EVENT_BUFFER, strlen(TRIGGER_EVENT_BUFFER));
    return 1;
}


/********************************************
 * Action
 ********************************************/

int get_actuator_command_switch(char *cmd){
    if (strcmp(cmd, SWITCH_VALUE_ON) == 0 || strcmp(cmd, SWITCH_VALUE_ACTIVE) == 0){
        return TA_ACTUATOR_CMD_ON;
    } else if (strcmp(cmd, SWITCH_VALUE_OFF) == 0 || strcmp(cmd, SWITCH_VALUE_INACTIVE) == 0){
        return TA_ACTUATOR_CMD_OFF;
    }
    return TA_ACTUATOR_CMD_SET;
}

int get_actuator_argument_switch(char *arg){
    if (strcmp(arg, SWITCH_ARG_WAIT) == 0 || strcmp(arg, SWITCH_ARG_SLEEP) == 0)
        return TA_ACTUATOR_ARG_SLEEP;
    else if (strcmp(arg, SWITCH_ARG_LEVEL) == 0)
        return TA_ACTUATOR_ARG_LEVEL;
    else if (strcmp(arg, SWITCH_ARG_MODE) == 0)
        return TA_ACTUATOR_ARG_MODE;

    return -1;
}