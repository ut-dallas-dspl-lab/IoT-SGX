#ifndef IOT_SGX_RULEPARSER_H
#define IOT_SGX_RULEPARSER_H


#include "Enclave_t.h"
#include "EnclaveHelper.h"

/* Keywords Macros for Rule Parsing */
#define	RULE_KEY_RULEID             "ruleID"
#define	RULE_KEY_CAPABILITY         "capability"
#define	RULE_KEY_ATTRIBUTE          "attribute"
#define	RULE_KEY_DEVICES            "devices"
#define	RULE_KEY_DEVICE             "device"
#define	RULE_KEY_ARGUMENTS          "arguments"
#define	RULE_KEY_COMMANDS           "commands"
#define	RULE_KEY_COMMAND            "command"
#define	RULE_KEY_ACTIONS            "actions"
#define	RULE_KEY_ACTION             "action"
#define	RULE_KEY_LEFT               "left"
#define	RULE_KEY_RIGHT              "right"
#define	RULE_KEY_IF                 "if"
#define	RULE_KEY_THEN               "then"
#define	RULE_KEY_STRING             "string"
#define	RULE_KEY_INTEGER            "integer"
#define	RULE_KEY_NUMBER             "number"

/* Keywords Macros for Event Parsing */
#define	EVENT_KEY_DEVICEID          "deviceID"
#define	EVENT_KEY_VALUE             "value"
#define	EVENT_KEY_UNIT              "unit"
#define	EVENT_KEY_TIMESTAMP         "timestamp"

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



RuleType parseRuleTypeAction(char *rule);
bool parseRule(char *rule, RuleType type, Rule *myRule);
bool parseDeviceEventData(char *event, DeviceEvent *deviceEvent);
bool parseDeviceInfo(char *info, network *net);

#endif //IOT_SGX_RULEPARSER_H
