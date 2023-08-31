/*
 * This file is specified for motion sensor devices.
 */

#ifndef OPTEE_EXAMPLE_IOT_DEVICE_MOTION_SENSOR_TA_H
#define OPTEE_EXAMPLE_IOT_DEVICE_MOTION_SENSOR_TA_H

#include <main_TA.h>

/* Capability */
#define	DEVICE_CAPABILITY_MOTION    "motion"
#define	DEVICE_CAPABILITY_MOTION_SENSOR    "motionsensor"
#define	DEVICE_ATTRIBUTE_MOTION    "motion"

/* Trigger: Device values */
#define	MOTION_SENSOR_VALUE_TRUE    "1"
#define	MOTION_SENSOR_VALUE_FALSE    "0"
#define	MOTION_SENSOR_VALUE_ON    "on"
#define	MOTION_SENSOR_VALUE_OFF    "off"
#define	MOTION_SENSOR_VALUE_ACTIVE    "active"
#define	MOTION_SENSOR_VALUE_INACTIVE    "inactive"

int prepare_trigger_for_motion_sensor(char *sensor_value, char **response, iot *device_properties);
int check_device_type_motion_sensor(char *capability);

#endif //OPTEE_EXAMPLE_IOT_DEVICE_MOTION_SENSOR_TA_H
