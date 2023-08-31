/*
 * This file is specified for switches or similar devices, such as light, LED, bulbs, etc.
 */
#ifndef OPTEE_EXAMPLE_IOT_DEVICE_SWITCH_H
#define OPTEE_EXAMPLE_IOT_DEVICE_SWITCH_H

int invoke_actuator_led(int command_type, int arg_type, char *arg);

#endif //OPTEE_EXAMPLE_IOT_DEVICE_SWITCH_H
