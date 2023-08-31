#include "include/device_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <main_TA.h>

#include "include/device_switch.h"


/********************************************
 * Manage Actuators
 ********************************************/

/*
 * Controls actuator invocation to respective files according to device_type
 */
int invoke_actuator(int device_type, int command_type, int arg_type, char *arg){
    printf("Calling Actuator...\n");

    /* Note: Plug in other actuator devices */
    switch (device_type) {
        case TA_DEVICE_TYPE_LED:
            return invoke_actuator_led(command_type, arg_type, arg);
    }
    return 0;
}