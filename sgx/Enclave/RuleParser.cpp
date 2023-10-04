#include "RuleParser.h"
#include "stdio.h"
#include "stdlib.h"
#include "string"
#include "vector"
#include <ctype.h>
#include "cJSON.h"
#include "Enclave.h"
#include "ObliviousOperationManager.h"


bool TEE_malloc(char **dest, char *src, int len){
    *dest = NULL;
    *dest = (char*) malloc(len + 1);
    //memcpy(*dest, src, len);
    (*dest)[len] = '\0';
    o_memcpy(1, *dest, src, len);
    bool isSuccess = memcmp(*dest, src, len) != 0 ? false : true;
    return isSuccess;
}

bool parseTriggerDevice(cJSON *triggerObj, Rule *myrule){
    //printf("RuleParser:: #parseTriggerDevice ");
    bool isValid = false;
    /* Capability */
    bool isCapabilityParsed = false;
    myrule->trigger->capability = NULL;
    const cJSON *capability = cJSON_GetObjectItem(triggerObj, RULE_KEY_CAPABILITY);
    isValid = cJSON_IsString(capability) && (capability->valuestring != NULL);
    isCapabilityParsed = isValid && TEE_malloc(&myrule->trigger->capability, capability->valuestring, strlen(capability->valuestring));

    /* Attribute */
    bool isAttributeParsed = false;
    myrule->trigger->attribute = NULL;
    const cJSON *attribute = cJSON_GetObjectItem(triggerObj, RULE_KEY_ATTRIBUTE);
    isValid = cJSON_IsString(attribute) && (attribute->valuestring != NULL);
    isAttributeParsed = isValid && TEE_malloc(&myrule->trigger->attribute, attribute->valuestring, strlen(attribute->valuestring));

    /* Device IDs */
    bool isDeviceIDParsed = false;
    myrule->trigger->deviceID = NULL;
    const cJSON *deviceList = cJSON_GetObjectItem(triggerObj, RULE_KEY_DEVICES);
    int numDevices = cJSON_IsArray(deviceList) ? cJSON_GetArraySize(deviceList) : 0;
    for (int i = 0 ; i < numDevices ; i++){
        isDeviceIDParsed = TEE_malloc(&myrule->trigger->deviceID, cJSON_GetArrayItem(deviceList, i)->valuestring, strlen(cJSON_GetArrayItem(deviceList, i)->valuestring));
        break;
    }
    bool isParseSuccess = isCapabilityParsed && isAttributeParsed && isDeviceIDParsed;
    return isParseSuccess;
}

bool parseActionDevice(const cJSON *actionCommand, Rule *myrule, RuleComponent *actionComponent, bool isElse){
    //printf("RuleParser:: #parseActionDevice ");
    bool isValid = false;
    /* Capability */
    bool isCapabilityParsed = false;
    //actionComponent->capability = NULL;
    const cJSON *capability = cJSON_GetObjectItem(actionCommand, RULE_KEY_CAPABILITY);
    isValid = cJSON_IsString(capability) && (capability->valuestring != NULL);
    isCapabilityParsed = isValid && TEE_malloc(isElse ? &myrule->actionElse->capability : &myrule->action->capability, capability->valuestring, strlen(capability->valuestring));

    /* Attribute */
    bool isAttributeParsed = false;
    //actionComponent->attribute = NULL;
    const cJSON *attribute = cJSON_GetObjectItem(actionCommand, RULE_KEY_COMMAND);
    isValid = cJSON_IsString(attribute) && (attribute->valuestring != NULL);
    isAttributeParsed = isValid && TEE_malloc(isElse ? &myrule->actionElse->attribute : &myrule->action->attribute, attribute->valuestring, strlen(attribute->valuestring));

    /* Arguments value section */
    bool isArgParsed = true;
    actionComponent->valueString = NULL;
    myrule->action->valueString = NULL;
    actionComponent->valueType = STRING;
    const cJSON *arguments = cJSON_GetObjectItem(actionCommand, RULE_KEY_ARGUMENTS);
    int numArgs = cJSON_IsArray(arguments) ? cJSON_GetArraySize(arguments) : 0;
    for (int i = 0 ; i < numArgs ; i++){
        const cJSON *valueObj = cJSON_GetArrayItem(arguments, i)->child;

        bool isValidString = cJSON_IsString(valueObj) && (valueObj->valuestring != NULL);
        isArgParsed = isValidString && TEE_malloc(isElse ? &myrule->actionElse->valueString : &myrule->action->valueString, valueObj->valuestring, strlen(valueObj->valuestring));

        bool isValidNumber = cJSON_IsNumber(valueObj);
        actionComponent->value = isValidNumber ? (float)valueObj->valuedouble : 0;

        isArgParsed = isArgParsed || isValidNumber;
        actionComponent->valueType = isValidString ? STRING : NUMBER;
        //printf("%d %d %d %d\n", isCapabilityParsed, isAttributeParsed, isArgParsed, isValidString);
        break;
    }
    bool isParseSuccess = isCapabilityParsed && isAttributeParsed && isArgParsed;
    return isParseSuccess;
}

bool parseConditionValue(const cJSON *triggerValue, Rule *myrule){
    //printf("RuleParser:: #parseConditionValue ");
    ValueType triggerValueType = getValueType(triggerValue->child->string);
    const cJSON *valueObj = NULL;

    bool isValueTypeString = obliviousSelectEq(triggerValueType, STRING, true, false);
    bool isValueTypeNumber = obliviousSelectEq(triggerValueType, NUMBER, true, false);
    bool isValueTypeInteger = obliviousSelectEq(triggerValueType, INTEGER, true, false);

    valueObj = isValueTypeString ? cJSON_GetObjectItemCaseSensitive(triggerValue, RULE_KEY_STRING) : cJSON_GetObjectItemCaseSensitive(triggerValue, RULE_KEY_NUMBER);
    valueObj = isValueTypeNumber ? cJSON_GetObjectItemCaseSensitive(triggerValue, RULE_KEY_NUMBER) : valueObj;
    valueObj = isValueTypeInteger ? cJSON_GetObjectItemCaseSensitive(triggerValue, RULE_KEY_INTEGER) : valueObj;

    myrule->trigger->valueType = isValueTypeString ? STRING : NUMBER;
    myrule->trigger->valueString = NULL;

    bool isValidString = false;
    bool isStringValueParsed = false;
    isValidString = isValueTypeString && cJSON_IsString(valueObj) && (valueObj->valuestring != NULL);
    isStringValueParsed = isValidString ? TEE_malloc(&myrule->trigger->valueString, valueObj->valuestring, strlen(valueObj->valuestring)) : TEE_malloc(&myrule->trigger->valueString, "TR_NULL", strlen("TR_NULL"));

    bool isValidNumber = false;
    isValidNumber = isValueTypeNumber && cJSON_IsNumber(valueObj);
    myrule->trigger->value = isValidNumber ? valueObj->valuedouble : 0;

    isValidNumber = !isValidNumber && isValueTypeInteger && cJSON_IsNumber(valueObj);
    myrule->trigger->value = isValidNumber ? valueObj->valueint : 0;

    bool isValueParsed = isStringValueParsed || isValidNumber;
    return isValueParsed;
}

bool parseRuleForTrigger(const cJSON *trigger, Rule *myRule){
    //printf("RuleParser:: #parseRuleForTrigger ");
    const cJSON *condition = trigger->child;
    OperatorType triggerOperatorType = getOperatorType(condition->string);
    myRule->trigger->operatorType = triggerOperatorType;

    const cJSON *triggerObj = NULL;
    const cJSON *triggerValue = NULL;

    const cJSON *trOptionLeft = cJSON_GetObjectItemCaseSensitive(condition, RULE_KEY_LEFT);
    const cJSON *trOptionRight = cJSON_GetObjectItemCaseSensitive(condition, RULE_KEY_RIGHT);

    bool is_op_eq = obliviousSelectEq(triggerOperatorType, EQ, true, false);
    triggerObj = is_op_eq ? trOptionLeft : trOptionRight;
    triggerValue = is_op_eq ? trOptionRight : trOptionLeft;
    bool isValid = !cJSON_IsNull(triggerObj) && !cJSON_IsNull(triggerValue);

    bool isSuccess = isValid && parseTriggerDevice(triggerObj->child, myRule) && parseConditionValue(triggerValue, myRule);
    return isSuccess;
}

bool parseRuleForActionList(const cJSON *actions, Rule *myrule, RuleComponent *actionComponent, bool isElse){
    //printf("RuleParser:: #parseRuleForActionList ");
    myrule->isElseExist = isElse;

    const cJSON *action = NULL;
    bool isParseSuccess = true;
    cJSON_ArrayForEach(action, actions){
        char *commandType = action->child->string;
        bool isValidCommand = strcmp(commandType, RULE_KEY_COMMAND) == 0;

        const cJSON *commandJson = cJSON_GetObjectItem(action, RULE_KEY_COMMAND);
        const cJSON *deviceList = cJSON_GetObjectItem(commandJson, RULE_KEY_DEVICES);
        bool isValidDevices = isValidCommand && !cJSON_IsNull(commandJson) && !cJSON_IsNull(deviceList) && cJSON_IsArray(deviceList);
        int numDevices = isValidDevices ? cJSON_GetArraySize(deviceList) : 0;

        bool isDeviceIdParsed = false;
        for (int i = 0 ; i < numDevices ; i++){
            isDeviceIdParsed = TEE_malloc(isElse ? &myrule->actionElse->deviceID : &myrule->action->deviceID, cJSON_GetArrayItem(deviceList, i)->valuestring, strlen(cJSON_GetArrayItem(deviceList, i)->valuestring));
            break;
        }

        bool isResponseParsed = false;
        cJSON *monitor = cJSON_CreateObject();
        bool isValid = cJSON_AddItemToObject(monitor, RULE_KEY_COMMANDS, cJSON_GetObjectItemCaseSensitive(commandJson, RULE_KEY_COMMANDS));
        //bool isValid = cJSON_AddItemToObject(monitor, RULE_KEY_COMMAND, cJSON_GetObjectItemCaseSensitive(action, RULE_KEY_COMMAND));
        char *str = isValid ? cJSON_Print(monitor) : NULL;
        isResponseParsed = TEE_malloc(isElse ? &myrule->responseCommandElse :  &myrule->responseCommand, str, strlen(str));
        cJSON_free(str);
        cJSON_free(monitor);

        const cJSON *actionCommand = NULL;
        const cJSON *commandList = cJSON_GetObjectItemCaseSensitive(commandJson, RULE_KEY_COMMANDS);
        bool isActionDeviceParsed = false;
        cJSON_ArrayForEach(actionCommand, commandList)
        {
            isActionDeviceParsed = parseActionDevice(actionCommand, myrule, actionComponent, isElse);
            break;
        }
        isParseSuccess = isDeviceIdParsed && isResponseParsed && isActionDeviceParsed;
        break;
    }
    return isParseSuccess;
}

bool parseRuleForAction(const cJSON *command, Rule *myrule, RuleType type){
    //printf("RuleParser:: #parseRuleForAction ");
    bool isRuleTypeIf = obliviousSelectEq(type, IF, true, false);
    const cJSON *condition = isRuleTypeIf ? command->child : command->child->next;
    bool isParseSuccess = true;
    condition = condition->next;
    while (condition){
        char *conditionType = condition->string;
        bool isConditionThen = isRuleTypeIf && obliviousStringCompare(conditionType, RULE_KEY_THEN) == 0;

        //RuleComponent *temp = isConditionThen ? myrule->action : myrule->actionElse;
        bool isSuccess = false;
        isSuccess = isConditionThen ? parseRuleForActionList(condition, myrule, myrule->action, isConditionThen) : parseRuleForActionList(condition, myrule, myrule->actionElse, isConditionThen);
//            myrule->action = isConditionThen ? temp : NULL;
//            myrule->actionElse = isConditionThen ? NULL : temp;

        isParseSuccess = isParseSuccess && isSuccess;
        condition = condition->next;
    }
    return isParseSuccess;
}



/***************************************************/
/* Rule */
/***************************************************/

RuleType parseRuleTypeAction(char *rule){
    //printf("RuleParser:: #isRuleTypeIFAction ");
    cJSON *rule_json = cJSON_Parse(rule);
    if (rule_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            printf("RuleParser:: Error before: %s\n", error_ptr);
        }
        cJSON_Delete(rule_json);
        return UNKNOWN_RULE;
    }
    const cJSON *action = NULL;
    const cJSON *actions = cJSON_GetObjectItemCaseSensitive(rule_json, RULE_KEY_ACTIONS);
    cJSON_ArrayForEach(action, actions)
    {
        char *actionType = action->child->string;
        //printf("action key: %s", actionType);
        RuleType rt = getRuleType(actionType);
        cJSON_Delete(rule_json);
        return rt;
    }
    cJSON_Delete(rule_json);
    return UNKNOWN_RULE;
}

bool parseRuleConditionIF(char *rule, Rule *myRule){
    //printf("RuleParser:: #parseRuleConditionIF ");
    cJSON *rule_json = cJSON_Parse(rule);
    if (rule_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            printf("RuleParser:: Error before: %s\n", error_ptr);
        }
        cJSON_Delete(rule_json);
        return false;
    }

    bool isValid = false;

    /* Rule ID */
    bool isRuleIDParsed = false;
    myRule->ruleID = NULL;
    const cJSON *ruleID = cJSON_GetObjectItemCaseSensitive(rule_json, RULE_KEY_RULEID);
    isValid = cJSON_IsString(ruleID) && (ruleID->valuestring != NULL);
    isRuleIDParsed = isValid && TEE_malloc(&myRule->ruleID, ruleID->valuestring, strlen(ruleID->valuestring));

    /* Rule body */
    bool isRuleBodyParsed = false;
    const cJSON *action = NULL;
    const cJSON *actions = cJSON_GetObjectItemCaseSensitive(rule_json, RULE_KEY_ACTIONS);
    cJSON_ArrayForEach(action, actions)
    {
        const cJSON *trigger = cJSON_GetObjectItem(action, RULE_KEY_IF);
        isRuleBodyParsed = parseRuleForTrigger(trigger, myRule) && parseRuleForAction(trigger, myRule, IF);
        break;
    }
    cJSON_Delete(rule_json);

    bool isParseSuccess = isRuleIDParsed && isRuleBodyParsed;
    return isParseSuccess;
}

bool parseRule(char *rule, RuleType type, Rule *myRule){
    //if (IS_DEBUG) printf("RuleParser:: #parseRule");
    myRule->ruleType = type;
    if(type == IF){
        return parseRuleConditionIF(rule, myRule);
    }
    /* Note: add other rule formats */

    return false;
}


/***************************************************/
/* Device Event */
/***************************************************/

/*
 * parseDeviceEventData:
 *  Parse the decrypted event string and make a Struct DeviceEvent
 *  @params: decrypted event string, Struct DeviceEvent
 *  returns: true if successful, else false
 */
bool parseDeviceEventData(char *event, DeviceEvent *deviceEvent){
    //printf("RuleParser:: #parseDeviceEventData ");
    //printf("Event = %s", event);

    cJSON *event_json = cJSON_Parse(event);

    if (event_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            printf("RuleParser:: Error before: %s\n", error_ptr);
        }
        cJSON_Delete(event_json);
        return false;
    }

    bool isValid = false;

    /* Device ID */
    bool isDeviceIDParsed = false;
    deviceEvent->deviceID = NULL;
    const cJSON *deviceId = cJSON_GetObjectItemCaseSensitive(event_json, EVENT_KEY_DEVICEID);
    isValid = cJSON_IsString(deviceId) && (deviceId->valuestring != NULL);
    isDeviceIDParsed = isValid && TEE_malloc(&deviceEvent->deviceID, deviceId->valuestring, strlen(deviceId->valuestring));
    //printf("RuleParser:: dest_id=%s, src_id=%s, isDeviceIDParsed=%d", deviceEvent->deviceID, deviceId->valuestring, isDeviceIDParsed);

    /* Capability */
    bool isCapabilityParsed = false;
    deviceEvent->capability = NULL;
    const cJSON *capability_json = event_json->child->next;
    isValid = cJSON_IsObject(capability_json) && !cJSON_IsNull(capability_json);
    isCapabilityParsed = isValid && TEE_malloc(&deviceEvent->capability, capability_json->string, strlen(capability_json->string));

    /* Attribute */
    bool isAttributeParsed = false;
    deviceEvent->attribute = NULL;
    const cJSON *attribute_json = capability_json->child;
    isValid = cJSON_IsObject(attribute_json) && !cJSON_IsNull(attribute_json);
    isAttributeParsed = isValid && TEE_malloc(&deviceEvent->attribute, attribute_json->string, strlen(attribute_json->string));

    /* Value */
    bool isValueParsed = false;
    deviceEvent->valueString = NULL;
    const cJSON *valueObj = cJSON_GetObjectItemCaseSensitive(attribute_json, EVENT_KEY_VALUE);
    isValid = !cJSON_IsNull(valueObj);

    bool isValidString = isValid && cJSON_IsString(valueObj) && (valueObj->valuestring != NULL);
    isValueParsed = isValidString ? TEE_malloc(&deviceEvent->valueString, valueObj->valuestring, obliviousStringLength(valueObj->valuestring)) : TEE_malloc(&deviceEvent->valueString, "EV_NULL", obliviousStringLength("EV_NULL"));

    bool isValidNumber = isValid && cJSON_IsNumber(valueObj);
    deviceEvent->value = isValidNumber ? (float)valueObj->valuedouble : 0;

    isValueParsed = isValueParsed || isValidNumber;
    deviceEvent->valueType = static_cast<ValueType>(obliviousSelectEq(isValidString, true, STRING, NUMBER));

    /* Unit */
    bool isUnitParsed = false;
    deviceEvent->unit = NULL;
    const cJSON *unitObj = cJSON_GetObjectItemCaseSensitive(attribute_json, EVENT_KEY_UNIT);
    isValid = !cJSON_IsNull(unitObj) && cJSON_IsString(unitObj) && (unitObj->valuestring != NULL);
    isUnitParsed = isValid && TEE_malloc(&deviceEvent->unit, unitObj->valuestring, strlen(unitObj->valuestring));

    /* Timestamp */
    bool isTimestampParsed = false;
    deviceEvent->timestamp = NULL;
    const cJSON *timestampObj = cJSON_GetObjectItemCaseSensitive(attribute_json, EVENT_KEY_TIMESTAMP);
    isValid = !cJSON_IsNull(timestampObj) && cJSON_IsString(timestampObj) && (timestampObj->valuestring != NULL);
    isTimestampParsed = isValid && TEE_malloc(&deviceEvent->timestamp, timestampObj->valuestring, strlen(timestampObj->valuestring));


    bool isParseSuccess = isDeviceIDParsed && isCapabilityParsed && isAttributeParsed && isValueParsed && isUnitParsed && isTimestampParsed;
    if (IS_DEBUG && !isParseSuccess) printf("RuleManager:: Event parsing failed!");

    cJSON_Delete(event_json);
    return isParseSuccess;
}

/*
 * buildActionCommand:
 *  Build a json string from the action command properties.
 *  @params: user ID, device ID, action command string
 *  returns: json string if successful, else NULL
 */
char* buildActionCommand(char *userID, char *deviceID, char *command){
    /* Create a cJSON Object */
    cJSON *monitor_json = cJSON_CreateObject();
    if (monitor_json == NULL){
        printf("Parser:: Failed to create cJSON Object.");
        return NULL;
    }

    /* Add User ID */
    if(cJSON_AddStringToObject(monitor_json, DEVICE_KEY_USERID, userID) == NULL){
        printf("Parser:: Failed to add device id.");
        cJSON_Delete(monitor_json);
        return NULL;
    }

    /* Add Device ID */
    cJSON *deviceID_array = cJSON_CreateArray();
    cJSON *deviceID_json = cJSON_CreateString(deviceID);
    if (deviceID_array == NULL || deviceID_json == NULL){
        printf("Parser:: Failed to create device id properties.");
        cJSON_Delete(monitor_json);
        return NULL;
    }

    if(cJSON_AddItemToArray(deviceID_array, deviceID_json) == NULL){
        printf("Parser:: Failed to add device id to array.");
        cJSON_Delete(monitor_json);
        return NULL;
    }

    if(cJSON_AddItemToObject(monitor_json, DEVICE_KEY_DEVICES, deviceID_array) == NULL){
        printf("Parser:: Failed to add device id array to json.");
        cJSON_Delete(monitor_json);
        return NULL;
    }

    /* Add Timestamp */
    size_t ts = ocallGetCurrentTime(SECOND);
    char ts_buffer[16];
    snprintf(ts_buffer,16, "%ld", ts);
    cJSON *ts_json = cJSON_CreateString(ts_buffer);
    if (ts_json == NULL){
        printf("Parser:: Failed to create timestamp properties.");
        cJSON_Delete(monitor_json);
        return NULL;
    }

    if(cJSON_AddItemToObject(monitor_json, "ts", ts_json) == NULL){
        printf("Parser:: Failed to add timestamp.");
        cJSON_Delete(monitor_json);
        return NULL;
    }

    /* Add Action command */
    cJSON *command_json = cJSON_Parse(command);
    if (command_json == NULL){
        printf("Parser:: Failed to create cJSON Object from command string");
        return NULL;
    }

    cJSON *cmd = cJSON_GetObjectItem(command_json, RULE_KEY_COMMANDS);
    if (!cJSON_IsArray(cmd) || cJSON_IsNull(cmd)){
        printf("Parser:: couldn't parse cmd!");
        cJSON_Delete(command_json);
        return NULL;
    }

    /* straight-forward, but issue with number format in TZ side of cjson */
//    if(cJSON_AddItemToObject(monitor_json, RULE_KEY_COMMANDS, cmd) == NULL){
//        printf("Parser:: Failed to add command.");
//        cJSON_Delete(monitor_json);
//        cJSON_Delete(command_json);
//        return NULL;
//    }

    cJSON *commands_array = cJSON_CreateArray();
    if(cJSON_AddItemToObject(monitor_json, RULE_KEY_COMMANDS, commands_array) == NULL){
        printf("Parser:: Failed to add commands array.");
        cJSON_Delete(monitor_json);
        cJSON_Delete(command_json);
        return NULL;
    }

    const cJSON *obj = NULL;
    cJSON_ArrayForEach(obj, cmd)
    {
        cJSON *element = cJSON_CreateObject();
        cJSON_AddItemToArray(commands_array, element);

        cJSON_AddStringToObject(element, "component", "main");

        const cJSON *capability = cJSON_GetObjectItem(obj, RULE_KEY_CAPABILITY);
        cJSON_AddStringToObject(element, RULE_KEY_CAPABILITY, capability->valuestring);

        const cJSON *ac_cmd = cJSON_GetObjectItem(obj, RULE_KEY_COMMAND);
        cJSON_AddStringToObject(element, RULE_KEY_COMMAND, ac_cmd->valuestring);

        const cJSON *arguments = cJSON_GetObjectItem(obj, RULE_KEY_ARGUMENTS);

        cJSON *args_array = cJSON_CreateArray();
        cJSON_AddItemToObject(element, RULE_KEY_ARGUMENTS, args_array);

        int numArgs = cJSON_IsArray(arguments) ? cJSON_GetArraySize(arguments) : 0;
        for (int i = 0 ; i < numArgs ; i++){
            const cJSON *arg = cJSON_GetArrayItem(arguments, i)->child;
            cJSON *arg_obj = cJSON_CreateObject();
            cJSON_AddItemToArray(args_array, arg_obj);

            if (cJSON_IsString(arg) && (arg->valuestring != NULL)){
                cJSON_AddStringToObject(arg_obj, arg->string, arg->valuestring);
            }else{
                char int_buffer[8];
                snprintf(int_buffer, 8, "%d", arg->valueint);
                cJSON_AddStringToObject(arg_obj, arg->string, int_buffer);
            }
        }
    }


    char *string = cJSON_Print(monitor_json);
    if (string == NULL){
        printf("Parser:: Failed to create string");
    }
    //printf("string = %s", string);

    //cJSON_Delete(command_json);
    cJSON_Delete(monitor_json);

    return string;
}


/***************************************************/
/* Device Info */
/***************************************************/

/*
 * parseDeviceInfo:
 *  Parse the json string to retrieve all the device settings and store in a Struct network
 *  @params: decrypted json string, Struct network
 *  returns: true if successful, else false
 */
bool parseDeviceInfo(char *info, network *net){
    bool status = false;

    // parse the JSON data
    cJSON *json = cJSON_Parse(info);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) printf("Error: %s", error_ptr);
        cJSON_Delete(json);
        return status;
    }

    /* user ID */
    cJSON *uid = cJSON_GetObjectItemCaseSensitive(json, DEVICE_KEY_USERID);
    if (!cJSON_IsString(uid) || (uid->valuestring == NULL)){
        printf("Parser:: couldn't parse userID!");
        cJSON_Delete(json);
        return status;
    }
    //printf("Checking uid: \"%s\"\n", uid->valuestring);
    net->userID = (char*) malloc((strlen(uid->valuestring)+1)* sizeof(char));
    memcpy(net->userID, uid->valuestring, strlen(uid->valuestring));
    net->userID[strlen(uid->valuestring)] = '\0';

    /* IoT devices */
    const cJSON *device = NULL;
    const cJSON *devices = NULL;
    devices = cJSON_GetObjectItemCaseSensitive(json, DEVICE_KEY_DEVICES);
    if (!cJSON_IsArray(devices) || cJSON_IsNull(devices)){
        printf("Parser::Error! Not an array!");
        cJSON_Delete(json);
        return status;
    }

    int num_devices = cJSON_GetArraySize(devices);
    net->n = num_devices;
    net->devices = (iot*) malloc(net->n * sizeof(iot));

    int count = 0;
    cJSON_ArrayForEach(device, devices){
        iot iot_device = {0};

        /* device ID */
        cJSON *did = cJSON_GetObjectItemCaseSensitive(device, DEVICE_KEY_DEVICEID);
        if (!cJSON_IsString(did) || (did->valuestring == NULL)){
            printf("Parser:: couldn't parse deviceID!");
            cJSON_Delete(json);
            return status;
        }
        //printf("Checking did: \"%s\"\n", did->valuestring);
        iot_device.deviceID = (char*) malloc((strlen(did->valuestring)+1)* sizeof(char));
        memcpy(iot_device.deviceID, did->valuestring, strlen(did->valuestring));
        iot_device.deviceID[strlen(did->valuestring)] = '\0';

        /* capability */
        cJSON *cap = cJSON_GetObjectItemCaseSensitive(device, DEVICE_KEY_CAPABILITY);
        if (!cJSON_IsString(cap) || (cap->valuestring == NULL)){
            printf("Parser:: couldn't parse capability!");
            cJSON_Delete(json);
            return status;
        }
        //printf("Checking cap: \"%s\"\n", cap->valuestring);
        iot_device.capability = (char*) malloc((strlen(cap->valuestring)+1)* sizeof(char));
        memcpy(iot_device.capability, cap->valuestring, strlen(cap->valuestring));
        iot_device.capability[strlen(cap->valuestring)] = '\0';

        /* attribute */
        cJSON *attr = cJSON_GetObjectItemCaseSensitive(device, DEVICE_KEY_ATTRIBUTE);
        if (!cJSON_IsString(attr) || (attr->valuestring == NULL)){
            printf("Parser:: couldn't parse attribute!");
            cJSON_Delete(json);
            return status;
        }
        //printf("Checking attr: \"%s\"\n", attr->valuestring);
        iot_device.attribute = (char*) malloc((strlen(attr->valuestring)+1)* sizeof(char));
        memcpy(iot_device.attribute, attr->valuestring, strlen(attr->valuestring));
        iot_device.attribute[strlen(attr->valuestring)] = '\0';

        /* unit */
        cJSON *unit = cJSON_GetObjectItemCaseSensitive(device, DEVICE_KEY_UNIT);
        if (!cJSON_IsString(unit) || (unit->valuestring == NULL)){
            printf("Parser:: couldn't parse unit!");
            cJSON_Delete(json);
            return status;
        }
        //printf("Checking unit: \"%s\"\n", unit->valuestring);
        iot_device.unit = (char*) malloc((strlen(unit->valuestring)+1)* sizeof(char));
        memcpy(iot_device.unit, unit->valuestring, strlen(unit->valuestring));
        iot_device.unit[strlen(unit->valuestring)] = '\0';

        /* mqtt topic */
        cJSON *topic = cJSON_GetObjectItemCaseSensitive(device, DEVICE_KEY_TOPIC);
        if (!cJSON_IsString(topic) || (topic->valuestring == NULL)){
            printf("Parser:: couldn't parse topic!");
            cJSON_Delete(json);
            return status;
        }
        //printf("Checking topic: \"%s\"\n", topic->valuestring);
        iot_device.mqtt_topic = (char*) malloc((strlen(topic->valuestring)+1)* sizeof(char));
        memcpy(iot_device.mqtt_topic, topic->valuestring, strlen(topic->valuestring));
        iot_device.mqtt_topic[strlen(topic->valuestring)] = '\0';

        /* mqtt pub/sub type */
        cJSON *mqtt_op = cJSON_GetObjectItemCaseSensitive(device, DEVICE_KEY_PUBSUB);
        if (!cJSON_IsString(mqtt_op) || (mqtt_op->valuestring == NULL)){
            printf("Parser:: couldn't parse pub/sub!");
            cJSON_Delete(json);
            return status;
        }
        //printf("Checking mqtt_op: \"%s\"\n", mqtt_op->valuestring);
        toLowerCase(mqtt_op->valuestring);
        iot_device.mqtt_op_type = (strcmp(mqtt_op->valuestring, DEVICE_KEY_PUBLISH) == 0) ? TA_MQTT_OPERATION_PUBLISH : TA_MQTT_OPERATION_SUBSCRIBE;


        /* TODO: add similar parsing for other device properties */


        net->devices[count] = iot_device;
        count++;
    }
    status = true; // successful parsing

    cJSON_Delete(json);
    return status;
}