#ifndef OPTEE_EXAMPLE_IOT_PARSER_TA_H
#define OPTEE_EXAMPLE_IOT_PARSER_TA_H

#include <main_TA.h>

int parse_device_info(char *info, network *net);
int parse_action_command(char *action, iot *event_properties, char **device_command, char **additional_arg, char **additional_arg_value);
int build_trigger_event(char **event_str, iot *device_prop, long ts);

#endif //OPTEE_EXAMPLE_IOT_PARSER_TA_H
