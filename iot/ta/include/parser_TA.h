#ifndef OPTEE_EXAMPLE_IOT_PARSER_TA_H
#define OPTEE_EXAMPLE_IOT_PARSER_TA_H

#include <main_TA.h>


/* Keywords Macros for Event Parsing */
#define	EVENT_KEY_DEVICEID          "deviceID"
#define	EVENT_KEY_VALUE             "value"
#define	EVENT_KEY_UNIT              "unit"
#define	EVENT_KEY_TIMESTAMP         "ts"

/* Keywords Macros for Action Event Parsing */
#define	ACTION_EVENT_KEY_CAPABILITY         "capability"
#define	ACTION_EVENT_KEY_ATTRIBUTE          "attribute"
#define	ACTION_EVENT_KEY_DEVICES            "devices"
#define	ACTION_EVENT_KEY_DEVICE             "device"
#define	ACTION_EVENT_KEY_ARGUMENTS          "arguments"
#define	ACTION_EVENT_KEY_COMMANDS           "commands"
#define	ACTION_EVENT_KEY_COMMAND            "command"

/* Keywords Macros for Device info Parsing */
#define	DEVICE_KEY_USERID           "userID"
#define	DEVICE_KEY_LOCATIONID       "locationID"
#define	DEVICE_KEY_DEVICES          "devices"
#define	DEVICE_KEY_DEVICEID         "deviceID"
#define	DEVICE_KEY_CAPABILITY       "capability"
#define	DEVICE_KEY_ATTRIBUTE        "attribute"
#define	DEVICE_KEY_UNIT             "unit"
#define	DEVICE_KEY_TOPIC            "topic"
#define	DEVICE_KEY_PUBSUB           "pub/sub"
#define	DEVICE_KEY_PUBLISH          "publish"
#define	DEVICE_KEY_SUBSCRIBE        "subscribe"

int parse_device_info(char *info, network *net);
int parse_action_command(char *action, iot *event_properties, char **device_command, char **additional_arg, char **additional_arg_value, long *timestamp);
int build_trigger_event(char **event_str, iot *device_prop, long ts);

#endif //OPTEE_EXAMPLE_IOT_PARSER_TA_H
