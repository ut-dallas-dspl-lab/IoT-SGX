#include "device_temp_sensor_TA.h"
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

int check_device_type_temperature_sensor(char *capability){
    if (strcmp(capability, DEVICE_CAPABILITY_TEMPERATURE) == 0 || strcmp(capability, DEVICE_CAPABILITY_TEMPERATURE_SENSOR) == 0 || strcmp(capability, DEVICE_CAPABILITY_TEMPERATURE_MEASUREMENT) == 0){
        return 1;
    }
    return 0;
}

int get_device_value_type_temperature_sensor(){
    return NUMBER;
}

/********************************************
 * Trigger
 ********************************************/

int prepare_trigger_for_temperature_sensor(char *sensor_value, char **response, iot *device_properties){
    memset(TRIGGER_EVENT_BUFFER, 0x0, sizeof(TRIGGER_EVENT_BUFFER));
    TEE_Time t;
    TEE_GetREETime(&t);

    device_properties->value = sensor_value;
    device_properties->value_type = get_device_value_type_temperature_sensor();

    build_trigger_event(response, device_properties, t.seconds);

    //sprintf(TRIGGER_EVENT_BUFFER, TRIGGER_EVENT_TEMPLATE, device_properties->deviceID, device_properties->capability, device_properties->attribute, device_properties->value, t.seconds);
    //TEE_MemMove((*response), TRIGGER_EVENT_BUFFER, strlen(TRIGGER_EVENT_BUFFER));
    return 1;
}