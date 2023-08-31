#include "device_motion_sensor_TA.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tee_internal_api.h>
#include "device_manager_TA.h"
#include "utils_TA.h"
#include "parser_TA.h"

/********************************************
 * Device info
 ********************************************/

int check_device_type_motion_sensor(char *capability){
    if (strcmp(capability, DEVICE_CAPABILITY_MOTION) == 0 || strcmp(capability, DEVICE_CAPABILITY_MOTION_SENSOR) == 0){
        return 1;
    }
    return 0;
}

int get_device_value_type_motion_sensor(){
    return STRING;
}

/********************************************
 * Trigger
 ********************************************/

int prepare_trigger_for_motion_sensor(char *sensor_value, char **response, iot *device_properties){
    memset(TRIGGER_EVENT_BUFFER, 0x0, sizeof(TRIGGER_EVENT_BUFFER));
    char *value = toLower(sensor_value);

    TEE_Time t;
    TEE_GetREETime(&t);
    //printf("Sys time from TA: %d\n", t.seconds);


    if(strcmp(value, MOTION_SENSOR_VALUE_TRUE) == 0 || strcmp(value, MOTION_SENSOR_VALUE_ON) == 0 || strcmp(value, MOTION_SENSOR_VALUE_ACTIVE) == 0){
        device_properties->value = MOTION_SENSOR_VALUE_ACTIVE;
    } else if (strcmp(value, MOTION_SENSOR_VALUE_FALSE) == 0 || strcmp(value, MOTION_SENSOR_VALUE_OFF) == 0 || strcmp(value, MOTION_SENSOR_VALUE_INACTIVE) == 0){
        device_properties->value = MOTION_SENSOR_VALUE_INACTIVE;
    }
    else{
        printf("Error! Unknown sensor value!\n");
        return 0;
    }
    device_properties->value_type = get_device_value_type_motion_sensor();

    build_trigger_event(response, device_properties, t.seconds);

    //sprintf(TRIGGER_EVENT_BUFFER, TRIGGER_EVENT_TEMPLATE, device_properties->deviceID, device_properties->capability, device_properties->attribute, device_properties->value, t.seconds);
    //TEE_MemMove((*response), TRIGGER_EVENT_BUFFER, strlen(TRIGGER_EVENT_BUFFER));
    return 1;
}

/********************************************
 * Action
 ********************************************/

