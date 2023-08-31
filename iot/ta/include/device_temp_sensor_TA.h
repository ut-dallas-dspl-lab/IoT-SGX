/*
 * This file is specified for temperature sensor devices.
 */

#ifndef OPTEE_EXAMPLE_IOT_DEVICE_TEMP_SENSOR_TA_H
#define OPTEE_EXAMPLE_IOT_DEVICE_TEMP_SENSOR_TA_H

#include <main_TA.h>

/* Capability */
#define	DEVICE_CAPABILITY_TEMPERATURE    "temperature"
#define	DEVICE_CAPABILITY_TEMPERATURE_SENSOR    "temperaturesensor"
#define	DEVICE_CAPABILITY_TEMPERATURE_MEASUREMENT    "temperaturemeasurement"


int check_device_type_temperature_sensor(char *capability);
int prepare_trigger_for_temperature_sensor(char *sensor_value, char **response, iot *device_properties);

#endif //OPTEE_EXAMPLE_IOT_DEVICE_TEMP_SENSOR_TA_H
