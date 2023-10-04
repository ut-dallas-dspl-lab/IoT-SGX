#include "EnclaveManager.h"
#include "Enclave.h"
#include "EnclaveHelper.h"
#include "RuleManager.h"
#include "analytics_utils.h"
#include "RuleParser.h"
#include "RuleConflictDetectionManager.h"
#include "EnclaveDatabaseManager.h"
#include "ObliviousOperationManager.h"

network net; /* Stores user and device information */

/******************************************************/
/* Setup */
/******************************************************/
void setupEnclave(){
    /* Do any initial setup here */
    net = {0};
}

/**
 * setupDeviceInfo:
 *  Parse device information from json string and store in a Global Struct Network
 *  @params: json string with device information
 */
int setupDeviceInfo(char *info){
    bool status = parseDeviceInfo(info, &net);
    if (!status){
        printf("EnclaveManager:: Device info parsing failed!");
        return 0;
    }

    for (int i = 0; i < net.n; ++i) {
        printf("#%d: did=%s, cap=%s, attr=%s, topic=%s, pub/sub=%d",i+1, net.devices[i].deviceID, net.devices[i].capability, net.devices[i].attribute, net.devices[i].mqtt_topic, net.devices[i].mqtt_op_type);
    }
    return 1;
}

/******************************************************/
/* Device Info */
/******************************************************/

/**
 * getNumSubscriptionTopics:
 *  Returns the number of MQTT Sub topics.
 */
int getNumSubscriptionTopics(){
    int count = 0;
    for (int i = 0; i < net.n; ++i) {
        if(net.devices[i].mqtt_op_type == TA_MQTT_OPERATION_PUBLISH) count++;
    }
    return count;
}

/**
 * getSubscriptionTopicNames:
 *  Returns the MQTT Sub topic names.
 *  @params: a struct Message, the number of MQTT Sub topics.
 */
int getSubscriptionTopicNames(struct Message *msg, size_t num_topics){
    int count = 0;
    for (int i = 0; i < net.n; ++i) {
        if(net.devices[i].mqtt_op_type == TA_MQTT_OPERATION_PUBLISH){
            memcpy(msg->text, net.devices[i].mqtt_topic, strlen(net.devices[i].mqtt_topic));
            msg->text[strlen(net.devices[i].mqtt_topic)] = '\0';
            count +=1;
            //printf("#%d = %s\n", count, msg->text);
            if (count == num_topics) break;
            msg++;
        }
    }
    return 1;
}

/**
 * getDeviceTopicName:
 *  Returns the MQTT topic name for a given device ID
 *  @params: device ID
 */
char *getDeviceTopicName(char *deviceID){
    for (int i = 0; i < net.n; ++i){
        if(obliviousStringCompare(deviceID, net.devices[i].deviceID)) {
            return net.devices[i].mqtt_topic;
        }
    }
    return NULL;
}

/******************************************************/
/* Rule */
/******************************************************/

/**
 * didReceiveRule:
 *  Manages incoming Rules
 *  @params: a struct Message containing the encrypted Rule
 */
int didReceiveRule(struct Message *msg){
    if(IS_DEBUG) printf("EnclaveManager:: didReceiveRule");
    size_t timeStart = 0;

    /* decrypt rule */
    if (IS_BENCHMARK) timeStart = ocallGetCurrentTime(MILLI);
    char *decMessage = (char *) malloc((msg->textLength+1)*sizeof(char));
    bool isDecryptSuccess = decryptMessage_AES_GCM_Tag(msg->text, msg->textLength, decMessage, msg->textLength, msg->tag);
    if (IS_BENCHMARK) benchmarkTimeAES(ocallGetCurrentTime(MILLI)-timeStart);

    /* parse rule */
    struct Rule *myrule;
    initRule(&myrule);
    bool isValid = isDecryptSuccess && startParsingRule(decMessage, myrule);

    /* conflict detection */
    if (IS_BENCHMARK) timeStart = ocallGetCurrentTime(MILLI);
    bool isValidRule = isValid && !startRuleConflictDetection(myrule);
    if (IS_BENCHMARK) benchmarkTimeRuleConflict(ocallGetCurrentTime(MILLI)-timeStart);

    /* create dummy rule */
    struct Rule *dummyRule;
    initRule(&dummyRule);
    createDummyRule(myrule, dummyRule);

    /* store in db */
    bool isStored = isValidRule ? storeRuleInDB(decMessage, myrule) : storeRuleInDB(decMessage, dummyRule);

    /* cleanup */
    deleteRule(&dummyRule);
    deleteRule(&myrule);
    if (decMessage != NULL) free(decMessage);

    return isValidRule && isStored ? 1 : -1;
}



/******************************************************/
/* Event */
/******************************************************/

/**
 * didReceiveEventMQTT:
 *  Manages incoming device events
 *  @params: a struct Message containing the encrypted event string
 */
void didReceiveEventMQTT(struct Message *msg) {
    if(IS_DEBUG) printf("EnclaveManager:: didReceiveEventMQTT");

    /* Incoming trigger event buffer=Tag+EncMsg. Split the buffer before decryption */
    int dec_msg_len = msg->textLength - SGX_AESGCM_MAC_SIZE;
    bool isValidEvent = dec_msg_len > 0;
    if (IS_DEBUG && !isValidEvent) printf("EnclaveManager:: Invalid MQTT payload length!");

    char *decMessage = (char *) malloc((dec_msg_len+1) * sizeof(char));
    bool isDecryptionSuccess = isValidEvent && decryptMessage_AES_GCM(msg->text, msg->textLength, decMessage, dec_msg_len);

    /* Parse trigger event */
    struct DeviceEvent *myEvent = (DeviceEvent*) malloc( sizeof( struct DeviceEvent));
    bool isEmptyEvent = myEvent == NULL;
    if (IS_DEBUG && isEmptyEvent) printf("EnclaveManager:: Memory allocation error!");

    bool isParsed = isDecryptionSuccess && !isEmptyEvent && parseDeviceEventData(decMessage, myEvent);
    if (IS_DEBUG && isParsed) printDeviceEventInfo(myEvent);

    /* Verify timestamp: invalid event if its timestamp vary more than the Threshold from current timestamp */
    bool isValidTimestamp = (msg->timestamp - atol(myEvent->timestamp)) <= TIMESTAMP_THRESHOLD_SEC;
    if (IS_DEBUG && !isValidTimestamp) {
        printf("current ts = %ld, event ts = %ld", msg->timestamp,  atol(myEvent->timestamp));
        printf("EnclaveManager:: Invalid Timestamp!");
    }

    /* Start rule automation */
    if(isParsed && isValidTimestamp) startRuleAutomation(myEvent);

    /* cleanup */
    deleteDeviceEvent(&myEvent);
    free(decMessage);
    return;
}


/**
 * sendDeviceCommands:
 *  sends action-commands to respective devices according to the Action part of the Rule
 *  @params: a Rule, a boolean indicating whether the rule is satisfied
 *  returns: true if command sent successfully, else false
 */
int sendDeviceCommands(Rule *myRule, bool isRuleSatisfied){
    TOTAL_DEVICE_COMMANDS += 1;
    if (IS_DEBUG) printf("isRuleSatisfied = %d", isRuleSatisfied);

    /* check if there is any 'else' response in the Rule */
    bool isEmptyResponse = obliviousSelectEq(isRuleSatisfied, 0) && obliviousSelectEq(myRule->isElseExist, 0);
    if (isEmptyResponse) return isRuleSatisfied;

    /* initialize variables */
    Message *response = (Message*) malloc(sizeof(Message));
    response->isEncrypted = IS_ENCRYPTION_ENABLED;

    char *deviceID = obliviousSelectEq(isRuleSatisfied, 1) ? myRule->action->deviceID : myRule->actionElse->deviceID;
    char *userID = net.userID;

    /* retrieve mqtt topic name */
    int isValidTopicName = 0;
    char *addr = getDeviceTopicName(deviceID);
    isValidTopicName = addr != NULL ? 1 : 0;
    int topicLen = strlen(addr);
    response->address = (char*) malloc( (topicLen+1) * sizeof(char));
    memcpy(response->address, addr, topicLen);
    response->address[topicLen] = '\0';

    /* make json response */
    char *command = obliviousSelectEq(isRuleSatisfied, 1) ? myRule->responseCommand : myRule->responseCommandElse;
    char *responseCommand = buildActionCommand(userID, deviceID, command);
    if (IS_DEBUG) printf("responseCommand = %s", responseCommand);

    /* encrypt the response */
    int len = obliviousStringLength(responseCommand);
    response->textLength = len+SGX_AESGCM_MAC_SIZE;
    response->text = (char *) malloc(sizeof(char) * (response->textLength+1));
    response->tag = NULL;
    bool isEncryptionSuccess = encryptMessage_AES_GCM(responseCommand, len, response->text, response->textLength);

    /* pass the response to the REE via ocall */
    int isValidResponse = obliviousAndOperation(isEncryptionSuccess, isValidTopicName);
    size_t isSuccess = 0;
    if (obliviousSelectEq(isValidResponse,1)) ocall_send_rule_commands_mqtt(&isSuccess, response->address, response->text, response->textLength);
    if (IS_DEBUG) isSuccess ? printf("RuleManager:: Successfully sent rule command to device!") : printf("RuleManager:: Failed to send rule command to device!");

    //TODO: verify decryption
    //char *dec_sample =  (char *) malloc(sizeof(char) * ((response->textLength - SGX_AESGCM_MAC_SIZE)+1));
    //decryptMessage_AES_GCM(response->text, response->textLength, dec_sample, response->textLength - SGX_AESGCM_MAC_SIZE);

    free(responseCommand);
    deleteMessage(&response);
    return isSuccess;
}