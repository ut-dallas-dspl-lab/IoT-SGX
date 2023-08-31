#include "include/device_switch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <main_TA.h>
#include <gpiod.h>
#define	SENSOR	"RPi3"

/********************************************
 * LED
 ********************************************/

int invoke_actuator_led(int command_type, int arg_type, char *arg){
    if (command_type != TA_ACTUATOR_CMD_ON && command_type != TA_ACTUATOR_CMD_OFF){
        printf("Error! LED command not supported\n");
        return 0;
    }

    char *chipname = "gpiochip0";
    unsigned int line_num = 17;	// GPIO Pin #
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    int ret, cmd;

    chip = gpiod_chip_open_by_name(chipname);
    if (!chip) {
        perror("Open chip failed\n");
        goto end;
    }

    line = gpiod_chip_get_line(chip, line_num);
    if (!line) {
        perror("Get line failed\n");
        goto close_chip;
    }

    ret = gpiod_line_request_output(line, SENSOR, 0);
    if (ret < 0) {
        perror("Request line as output failed\n");
        goto release_line;
    }

    cmd = command_type;

    /* Operate on the LED */
    ret = gpiod_line_set_value(line, cmd);
    if (ret < 0) {
        perror("Set line output failed\n");
        goto release_line;
    }
    printf("Output %u on line #%u\n", command_type, line_num);

    if (arg_type == TA_ACTUATOR_ARG_SLEEP){
        int sleep_time = atoi(arg);
        sleep(sleep_time);
        ret = gpiod_line_set_value(line, !cmd);
    }

    release_line:
    gpiod_line_release(line);
    close_chip:
    gpiod_chip_close(chip);
    end:
    return 0;
}


/********************************************
 * Light
 ********************************************/




/********************************************
 * Switch
 ********************************************/