/*
 * This file is specified for switches or similar devices, such as light, LED, bulbs, etc.
 */

#ifndef OPTEE_EXAMPLE_IOT_DEVICE_SWITCH_TA_H
#define OPTEE_EXAMPLE_IOT_DEVICE_SWITCH_TA_H

#include <main_TA.h>

/* Capability */
#define	DEVICE_CAPABILITY_SWITCH   "switch"
#define	DEVICE_CAPABILITY_LIGHT    "light"
#define	DEVICE_CAPABILITY_LED    "led"

/* Trigger: Device values */
#define SWITCH_VALUE_TRUE   "1"
#define SWITCH_VALUE_FALSE   "0"
#define SWITCH_VALUE_ON   "on"
#define SWITCH_VALUE_OFF   "off"
#define SWITCH_VALUE_ACTIVE   "active"
#define SWITCH_VALUE_INACTIVE   "inactive"

/* Action: Arguments */
#define SWITCH_ARG_LEVEL   "level"
#define SWITCH_ARG_MODE   "mode"
#define SWITCH_ARG_WAIT   "wait"
#define SWITCH_ARG_SLEEP   "sleep"

int prepare_trigger_for_switch(char *sensor_value, char **response, iot *device_properties);

int check_device_type_switch(char *capability);
int get_device_type_switch(char *capability);

int get_actuator_command_switch(char *cmd);
int get_actuator_argument_switch(char *arg);

#endif //OPTEE_EXAMPLE_IOT_DEVICE_SWITCH_TA_H
