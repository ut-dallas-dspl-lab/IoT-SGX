#include "EnclaveHelper.h"
#include "Enclave_t.h"
#include <stdio.h> /* vsnprintf */
#include <string.h>
#include <ctype.h>
#include "sgx_trts.h"


bool IS_ENCRYPTION_ENABLED = true;
bool IS_DEBUG = false;
bool IS_BENCHMARK = false;
bool IS_CFI_ENABLED = false;
bool IS_DATA_OBLIVIOUSNESS_ENABLED = true;
bool IS_PADDED = true;

int TOTAL_CONFLICTED_RULES = 0;
int TOTAL_RETRIEVED_RULES = 0;
int TOTAL_DEVICE_COMMANDS = 0;

int TIME_AES_METHOD = 0;
int COUNT_AES_METHOD = 0;
int TIME_RULE_CONFLICT_DETECTION = 0;
int COUNT_RULE_CONFLICT_DETECTION = 0;
int TIME_AUTOMATION = 0;
int COUNT_AUTOMATION = 0;

/****************************************************
 * Utility functions
 ****************************************************/
void toLowerCase(char *str){
    int i, s = strlen(str);
    for (i = 0; i < s; i++)
        str[i] = tolower(str[i]);
}

int generateRandomNumber(int limit){
    unsigned char alpha[5];
    sgx_status_t ret = sgx_read_rand(alpha, 5);
    if(ret != SGX_SUCCESS){
        printf("sgx_read_rand failed!");
        return 0;
    }

    //printf("sgx_read_rand: %u\n", (unsigned int)alpha[0]);
    int num = 0;
    for (int i = 0; i < 5; ++i) num += (unsigned int)alpha[i];
    num = num%limit;
    //printf("sgx_read_rand number: %d", num);

    return num;
}


/****************************************************
 * Timer Helpers
 ****************************************************/

TimeUnitType getTimeUnitType (char *key){
    toLowerCase(key);
    if(strcmp(key, "second")==0){
        return SECOND;
    } else if(strcmp(key, "minute")==0){
        return MINUTE;
    } else if(strcmp(key, "hour")==0){
        return HOUR;
    } else if(strcmp(key, "day")==0){
        return DAY;
    } else if(strcmp(key, "week")==0){
        return WEEK;
    } else if(strcmp(key, "month")==0){
        return MONTH;
    } else if(strcmp(key, "year")==0){
        return YEAR;
    } else {
        return UNKNOWN_TIME_UNIT;
    }
}

int getTimeMinute(int h, int m){
    return h*60+m;
}

int getTimeSecond(int h, int m){
    return (h*60+m)*60;
}



/****************************************************
 * Init and Delete Structs
 ****************************************************/

/*
 * Initialize struct Rule and its trigger & Action component
 */
bool initRule(Rule **myrule){
    *myrule = (Rule*) malloc( sizeof( struct Rule ));
    if (*myrule == NULL) {
        printf("EnclaveHelper:: Memory allocation error!");
        return false;
    }
    (*myrule)->trigger = (RuleComponent*) malloc( sizeof( struct RuleComponent ));
    if ((*myrule)->trigger == NULL) {
        printf("EnclaveHelper:: Memory allocation error!");
        return false;
    }
    (*myrule)->action = (RuleComponent*) malloc( sizeof( struct RuleComponent ));
    if ((*myrule)->action == NULL) {
        printf("EnclaveHelper:: Memory allocation error!");
        return false;
    }
    (*myrule)->actionElse = (RuleComponent*) malloc( sizeof( struct RuleComponent ));
    if ((*myrule)->actionElse == NULL) {
        printf("EnclaveHelper:: Memory allocation error!");
        return false;
    }
    (*myrule)->responseCommand = NULL;
    (*myrule)->responseCommandElse = NULL;
    (*myrule)->isElseExist = 0;

    return true;
}

/*
 * Copy struct Rule to struct newRule
 */
bool copyRule(Rule **rule, Rule **newRule){
    if ((*rule)->ruleID != NULL) {
        (*newRule)->ruleID = (char *) malloc(sizeof(char) * (strlen((*rule)->ruleID)+1));
        memcpy((*newRule)->ruleID, (*rule)->ruleID, strlen((*rule)->ruleID));
        (*newRule)->ruleID[strlen((*rule)->ruleID)] = '\0';
    }
    if ((*rule)->responseCommand != NULL) {
        (*newRule)->responseCommand = (char *) malloc(sizeof(char) * (strlen((*rule)->responseCommand)+1));
        memcpy((*newRule)->responseCommand, (*rule)->responseCommand, strlen((*rule)->responseCommand));
        (*newRule)->responseCommand[strlen((*rule)->responseCommand)] = '\0';
    }
    if ((*rule)->responseCommandElse != NULL) {
        (*newRule)->responseCommandElse = (char *) malloc(sizeof(char) * (strlen((*rule)->responseCommandElse)+1));
        memcpy((*newRule)->responseCommandElse, (*rule)->responseCommandElse, strlen((*rule)->responseCommandElse));
        (*newRule)->responseCommandElse[strlen((*rule)->responseCommandElse)] = '\0';
    }
    if ((*rule)->trigger != NULL) {
        //printf("EnclaveHelper:: copyRule: trigger!");
        if ((*rule)->trigger->deviceID != NULL) {
            //printf("EnclaveHelper:: copyRule: trigger->deviceID!");
            (*newRule)->trigger->deviceID = (char *) malloc(sizeof(char) * (strlen((*rule)->trigger->deviceID)+1));
            memcpy((*newRule)->trigger->deviceID, (*rule)->trigger->deviceID, strlen((*rule)->trigger->deviceID));
            (*newRule)->trigger->deviceID[strlen((*rule)->trigger->deviceID)] = '\0';
        }else{
            (*newRule)->trigger->deviceID = NULL;
        }
        if ((*rule)->trigger->capability != NULL) {
            //printf("EnclaveHelper:: copyRule: trigger->capability!");
            (*newRule)->trigger->capability = (char *) malloc(sizeof(char) * (strlen((*rule)->trigger->capability)+1));
            memcpy((*newRule)->trigger->capability, (*rule)->trigger->capability, strlen((*rule)->trigger->capability));
            (*newRule)->trigger->capability[strlen((*rule)->trigger->capability)] = '\0';
        }else{
            (*newRule)->trigger->capability = NULL;
        }
        if ((*rule)->trigger->attribute != NULL) {
            //printf("EnclaveHelper:: copyRule: trigger->attribute!");
            (*newRule)->trigger->attribute = (char *) malloc(sizeof(char) * (strlen((*rule)->trigger->attribute)+1));
            memcpy((*newRule)->trigger->attribute, (*rule)->trigger->attribute, strlen((*rule)->trigger->attribute));
            (*newRule)->trigger->attribute[strlen((*rule)->trigger->attribute)] = '\0';
        }else{
            (*newRule)->trigger->attribute = NULL;
        }
        if ((*rule)->trigger->valueString != NULL) {
            //printf("EnclaveHelper:: copyRule: trigger->valueString!");
            (*newRule)->trigger->valueString = (char *) malloc(sizeof(char) * (strlen((*rule)->trigger->valueString)+1));
            memcpy((*newRule)->trigger->valueString, (*rule)->trigger->valueString, strlen((*rule)->trigger->valueString));
            (*newRule)->trigger->valueString[strlen((*rule)->trigger->valueString)] = '\0';
        } else{
            (*newRule)->trigger->valueString = NULL;
        }
        (*newRule)->trigger->operatorType = (*rule)->trigger->operatorType;
        (*newRule)->trigger->valueType = (*rule)->trigger->valueType;
        (*newRule)->trigger->value = (*rule)->trigger->value;
    }

    if ((*rule)->action != NULL){
        //printf("EnclaveHelper:: copyRule: action!");
        if ((*rule)->action->deviceID != NULL) {
            //printf("EnclaveHelper:: copyRule: action->deviceID!");
            (*newRule)->action->deviceID = (char *) malloc(sizeof(char) * (strlen((*rule)->action->deviceID)+1));
            memcpy((*newRule)->action->deviceID, (*rule)->action->deviceID, strlen((*rule)->action->deviceID));
            (*newRule)->action->deviceID[strlen((*rule)->action->deviceID)] = '\0';
        } else{
            (*newRule)->action->deviceID = NULL;
        }
        if ((*rule)->action->capability != NULL) {
            //printf("EnclaveHelper:: copyRule: action->capability!");
            (*newRule)->action->capability = (char *) malloc(sizeof(char) * (strlen((*rule)->action->capability)+1));
            memcpy((*newRule)->action->capability, (*rule)->action->capability, strlen((*rule)->action->capability));
            (*newRule)->action->capability[strlen((*rule)->action->capability)] = '\0';
        }else{
            (*newRule)->action->capability = NULL;
        }
        if ((*rule)->action->attribute != NULL) {
            //printf("EnclaveHelper:: copyRule: action->attribute!");
            (*newRule)->action->attribute = (char *) malloc(sizeof(char) * (strlen((*rule)->action->attribute)+1));
            memcpy((*newRule)->action->attribute, (*rule)->action->attribute, strlen((*rule)->action->attribute));
            (*newRule)->action->attribute[strlen((*rule)->action->attribute)] = '\0';
        }else{
            (*newRule)->action->attribute = NULL;
        }
        if ((*rule)->action->valueString != NULL) {
            //printf("EnclaveHelper:: copyRule: action->valueString!");
            (*newRule)->action->valueString = (char *) malloc(sizeof(char) * (strlen((*rule)->action->valueString)+1));
            memcpy((*newRule)->action->valueString, (*rule)->action->valueString, strlen((*rule)->action->valueString));
            (*newRule)->action->valueString[strlen((*rule)->action->valueString)] = '\0';
        }else{
            (*newRule)->action->valueString = NULL;
        }
        (*newRule)->action->operatorType = (*rule)->action->operatorType;
        (*newRule)->action->valueType = (*rule)->action->valueType;
        (*newRule)->action->value = (*rule)->action->value;
    }

    if ((*rule)->actionElse != NULL){
        //printf("EnclaveHelper:: copyRule: actionElse!");
        if ((*rule)->actionElse->deviceID != NULL) {
            //printf("EnclaveHelper:: copyRule: actionElse->deviceID!");
            (*newRule)->actionElse->deviceID = (char *) malloc(sizeof(char) * (strlen((*rule)->actionElse->deviceID)+1));
            memcpy((*newRule)->actionElse->deviceID, (*rule)->actionElse->deviceID, strlen((*rule)->actionElse->deviceID));
            (*newRule)->actionElse->deviceID[strlen((*rule)->actionElse->deviceID)] = '\0';
        }else{
            (*newRule)->actionElse->deviceID = NULL;
        }
        if ((*rule)->actionElse->capability != NULL) {
            //printf("EnclaveHelper:: copyRule: actionElse->capability!");
            (*newRule)->actionElse->capability = (char *) malloc(sizeof(char) * (strlen((*rule)->actionElse->capability)+1));
            memcpy((*newRule)->actionElse->capability, (*rule)->actionElse->capability, strlen((*rule)->actionElse->capability));
            (*newRule)->actionElse->capability[strlen((*rule)->actionElse->capability)] = '\0';
        }else{
            (*newRule)->actionElse->capability = NULL;
        }
        if ((*rule)->actionElse->attribute != NULL) {
            //printf("EnclaveHelper:: copyRule: actionElse->attribute!");
            (*newRule)->actionElse->attribute = (char *) malloc(sizeof(char) * (strlen((*rule)->actionElse->attribute)+1));
            memcpy((*newRule)->actionElse->attribute, (*rule)->actionElse->attribute, strlen((*rule)->actionElse->attribute));
            (*newRule)->actionElse->attribute[strlen((*rule)->actionElse->attribute)] = '\0';
        }else{
            (*newRule)->actionElse->attribute = NULL;
        }
        if ((*rule)->actionElse->valueString != NULL) {
            //printf("EnclaveHelper:: copyRule: actionElse->valueString!");
            (*newRule)->actionElse->valueString = (char *) malloc(sizeof(char) * (strlen((*rule)->actionElse->valueString)+1));
            memcpy((*newRule)->actionElse->valueString, (*rule)->actionElse->valueString, strlen((*rule)->actionElse->valueString));
            (*newRule)->actionElse->valueString[strlen((*rule)->actionElse->valueString)] = '\0';
        }else{
            (*newRule)->actionElse->valueString = NULL;
        }
        (*newRule)->actionElse->operatorType = (*rule)->actionElse->operatorType;
        (*newRule)->actionElse->valueType = (*rule)->actionElse->valueType;
        (*newRule)->actionElse->value = (*rule)->actionElse->value;
    }
    return true;
}

/*
 * Delete struct Rule and its properties
 */
void deleteRule(Rule **myrule){
    if (*myrule == NULL) {
        printf("EnclaveHelper:: Rule NULL!");
        return;
    }

    if ((*myrule)->trigger != NULL) {
        //printf("EnclaveHelper:: trigger!");
        if ((*myrule)->trigger->deviceID != NULL) free((*myrule)->trigger->deviceID);
        //printf("EnclaveHelper:: capability!");
        if ((*myrule)->trigger->capability != NULL) free((*myrule)->trigger->capability);
        //printf("EnclaveHelper:: attribute!");
        if ((*myrule)->trigger->attribute != NULL) free((*myrule)->trigger->attribute);
        //printf("EnclaveHelper:: valueString!");
        if ((*myrule)->trigger->valueString != NULL) free((*myrule)->trigger->valueString);
        free((*myrule)->trigger);
        (*myrule)->trigger = NULL;
    }
    if ((*myrule)->action != NULL){
        //printf("EnclaveHelper:: action!");
        if ((*myrule)->action->deviceID != NULL) free((*myrule)->action->deviceID);
        //printf("EnclaveHelper:: capability!");
        if ((*myrule)->action->capability != NULL) free((*myrule)->action->capability);
        //printf("EnclaveHelper:: attribute!");
        if ((*myrule)->action->attribute != NULL) free((*myrule)->action->attribute);
        //printf("EnclaveHelper:: valueString!");
        if ((*myrule)->action->valueString != NULL) free((*myrule)->action->valueString);
        //printf("EnclaveHelper:: action full!");
        free((*myrule)->action);
        (*myrule)->action = NULL;
    }
    if ((*myrule)->actionElse != NULL){
        //printf("EnclaveHelper:: actionElse!");
        if ((*myrule)->actionElse->deviceID != NULL) free((*myrule)->actionElse->deviceID);
        //printf("EnclaveHelper:: capability!");
        if ((*myrule)->actionElse->capability != NULL) free((*myrule)->actionElse->capability);
        //printf("EnclaveHelper:: attribute!");
        if ((*myrule)->actionElse->attribute != NULL) free((*myrule)->actionElse->attribute);
        //printf("EnclaveHelper:: valueString!");
        if ((*myrule)->actionElse->valueString != NULL) free((*myrule)->actionElse->valueString);
        //printf("EnclaveHelper:: actionElse full!");
        free((*myrule)->actionElse);
        (*myrule)->actionElse = NULL;
    }
    //printf("EnclaveHelper:: delete ruleID!");
    if ((*myrule)->ruleID != NULL) free((*myrule)->ruleID);
    //printf("EnclaveHelper:: delete responseCommand!");
    if ((*myrule)->responseCommand != NULL) free((*myrule)->responseCommand);
    //printf("EnclaveHelper:: delete responseCommandElse!");
    if ((*myrule)->responseCommandElse != NULL) free((*myrule)->responseCommandElse);
    //printf("EnclaveHelper:: delete myrule!");
    if(*myrule != NULL ) free(*myrule);
    *myrule = NULL;
    //return true;
}

/*
 * Delete struct DeviceEvent and its properties
 */
void deleteDeviceEvent(DeviceEvent **myEvent){
    if (*myEvent == NULL) {
        printf("EnclaveHelper:: Device Event NULL!");
        return;
    }
    if((*myEvent)->deviceID != NULL) free((*myEvent)->deviceID);
    //printf("EnclaveHelper:: capability!");
    if((*myEvent)->capability != NULL) free((*myEvent)->capability);
    //printf("EnclaveHelper:: attribute!");
    if((*myEvent)->attribute != NULL) free((*myEvent)->attribute);
    //printf("EnclaveHelper:: valueString!");
    if((*myEvent)->valueString != NULL) free((*myEvent)->valueString);
    //printf("EnclaveHelper:: unit!");
    if((*myEvent)->unit != NULL) free((*myEvent)->unit);
    //printf("EnclaveHelper:: timestamp!");
    if((*myEvent)->timestamp != NULL) free((*myEvent)->timestamp);
    //printf("EnclaveHelper:: freeing event!");
    free(*myEvent);
    *myEvent = NULL;
    //return true;
}

/*
 * Delete struct Message and its properties
 */
void deleteMessage(Message **msg){
    if((*msg)->text != NULL){
        //printf("EnclaveHelper:: text!");
        free((*msg)->text);
    }
    if((*msg)->tag != NULL) {
        //printf("EnclaveHelper:: tag!");
        free((*msg)->tag);
    }
    if((*msg) != NULL){
        //printf("EnclaveHelper:: freeing msg!");
        free(*msg);
        *msg = NULL;
    }
    //return true;
}

/*
 * Delete struct DatabaseElement and its properties
 */
void deleteDbElement(DatabaseElement **dbElement){
    if((*dbElement)->data->text != NULL){
        //printf("EnclaveHelper:: text!");
        free((*dbElement)->data->text);
    }
    if((*dbElement)->data->tag != NULL) {
        //printf("EnclaveHelper:: tag!");
        free((*dbElement)->data->tag);
    }
    if((*dbElement)->data != NULL){
        //printf("EnclaveHelper:: data!");
        free((*dbElement)->data);
    }
    if((*dbElement) != NULL){
        //printf("EnclaveHelper:: dbElement!");
        free((*dbElement));
        *dbElement = NULL;
    }
    //return true;
}


/****************************************************
 * Methods for Enums
 ****************************************************/
RuleType getRuleType(char *key){
    toLowerCase(key);
    if(strcmp(key, "if")==0){
        return IF;
    } else if(strcmp(key, "every")==0){
        return EVERY;
    } else{
        return UNKNOWN_RULE;
    }
}

ValueType getValueType(char *key){
    toLowerCase(key);
    if(strcmp(key, "string")==0){
        return STRING;
    }else if(strcmp(key, "integer")==0){
        return INTEGER;
    } else if(strcmp(key, "number")==0){
        return NUMBER;
    } else{
        return UNKNOWN_VALUE;
    }
}

OperatorType getOperatorType (char *key){
    if(strcmp(key, "equals")==0 || strcmp(key, "equal")==0){
        return EQ;
    }else if(strcmp(key, "greater_than")==0 || strcmp(key, "gt")==0){
        return GT;
    }
    else if(strcmp(key, "less_than")==0 || strcmp(key, "lt")==0){
        return LT;
    }
    else if(strcmp(key, "greater_than_or_equals")==0 || strcmp(key, "greater_than_or_equal")==0 || strcmp(key, "gte")==0){
        return GTE;
    }
    else if(strcmp(key, "less_than_or_equals")==0 || strcmp(key, "less_than_or_equal")==0 || strcmp(key, "lte")==0){
        return LTE;
    }
    else{
        return INVALID_OP;
    }
}


/****************************************************
 * Printing
 ****************************************************/

/*
 * Print Rule information
 */
void printRuleInfo(Rule *myRule){
    if(myRule->ruleType == IF){
        if(myRule->trigger->valueType == STRING && myRule->action->valueType == STRING){
            printf("Rule:: id=%s, tr_device_id=%s, tr_cap=%s, tr_attr=%s, tr_value=%s, tr_valtype=%d, tr_op=%d; ac_device_id=%s, ac_cap=%s, ac_attr=%s, ac_value=%s, ac_valtype=%d",
                   myRule->ruleID, myRule->trigger->deviceID, myRule->trigger->capability, myRule->trigger->attribute, myRule->trigger->valueString, myRule->trigger->valueType, myRule->trigger->operatorType,
                   myRule->action->deviceID, myRule->action->capability, myRule->action->attribute, myRule->action->valueString, myRule->action->valueType);

        }
        else if (myRule->trigger->valueType == NUMBER && myRule->action->valueType == NUMBER){
            printf("Rule:: id=%s, tr_device_id=%s, tr_cap=%s, tr_attr=%s, tr_value=%f, tr_valtype=%d, tr_op=%d; ac_device_id=%s, ac_cap=%s, ac_attr=%s, ac_value=%f, ac_valtype=%d",
                   myRule->ruleID, myRule->trigger->deviceID, myRule->trigger->capability, myRule->trigger->attribute, myRule->trigger->value, myRule->trigger->valueType, myRule->trigger->operatorType,
                   myRule->action->deviceID, myRule->action->capability, myRule->action->attribute, myRule->action->value, myRule->action->valueType);
        }
        else if (myRule->trigger->valueType == STRING && myRule->action->valueType == NUMBER){
            printf("Rule:: id=%s, tr_device_id=%s, tr_cap=%s, tr_attr=%s, tr_value=%s, tr_valtype=%d, tr_op=%d; ac_device_id=%s, ac_cap=%s, ac_attr=%s, ac_value=%f, ac_valtype=%d",
                   myRule->ruleID, myRule->trigger->deviceID, myRule->trigger->capability, myRule->trigger->attribute, myRule->trigger->valueString, myRule->trigger->valueType, myRule->trigger->operatorType,
                   myRule->action->deviceID, myRule->action->capability, myRule->action->attribute, myRule->action->value, myRule->action->valueType);
        }
        else if (myRule->trigger->valueType == NUMBER && myRule->action->valueType == STRING){
            printf("Rule:: id=%s, tr_device_id=%s, tr_cap=%s, tr_attr=%s, tr_value=%f, tr_valtype=%d, tr_op=%d; ac_device_id=%s, ac_cap=%s, ac_attr=%s, ac_value=%s, ac_valtype=%d",
                   myRule->ruleID, myRule->trigger->deviceID, myRule->trigger->capability, myRule->trigger->attribute, myRule->trigger->value, myRule->trigger->valueType, myRule->trigger->operatorType,
                   myRule->action->deviceID, myRule->action->capability, myRule->action->attribute, myRule->action->valueString, myRule->action->valueType);
        }
        else{
            printf("Error! Something went wrong while trying to print!");
        }
    } else {
        printf("Error! Unknown rule type");
    }
}

/*
 * Print device event information
 */
void printDeviceEventInfo(DeviceEvent *myEvent){
    if(myEvent->valueType == STRING){
        printf("DeviceEvent:: Event: deviceID=%s, capability=%s, attribute=%s, value=%s, valueType=%d", myEvent->deviceID, myEvent->capability, myEvent->attribute, myEvent->valueString, myEvent->valueType);
    } else{
        printf("DeviceEvent:: Event: deviceID=%s, capability=%s, attribute=%s, value=%f, valueType=%d", myEvent->deviceID, myEvent->capability, myEvent->attribute, myEvent->value, myEvent->valueType);
    }
}

/*
 * Print the Rule conflict name
 */
void printConflictType(ConflictType conflict){
    switch (conflict){
        case SHADOW:{
            printf("RuleConflict:: Shadow!");
            break;
        }
        case EXECUTION:{
            printf("RuleConflict:: Execution!");
            break;
        }
        case MUTUAL:{
            printf("RuleConflict:: Environment Mutual Conflict!");
            break;
        }
        case DEPENDENCE:{
            printf("RuleConflict:: Dependence!");
            break;
        }
        case CHAIN:{
            printf("RuleConflict:: Chaining exist!");
            break;
        }
        case CHAIN_FWD:{
            printf("RuleConflict:: Forward Chaining exist!");
            break;
        }
        default:
            printf("RuleConflict: No conflict detected!");
    }
}


/****************************************************
 * Error functions
 ****************************************************/
/*
 * Print the type of sgx error and its code
 * @params: sgx status of a process
 */
void check_error_code(sgx_status_t stat){
    //printf("STATUS: %d",stat);
    /* More Error Codes in SDK Developer Reference */
    switch (stat){
        case SGX_SUCCESS:
            printf("SGX_SUCCESS, with code=%d",stat);
            break;
        case SGX_ERROR_INVALID_PARAMETER:
            printf("SGX_ERROR_INVALID_PARAMETER, with code=%d", stat);
            break;
        case SGX_ERROR_MAC_MISMATCH:
            printf("SGX_ERROR_MAC_MISMATCH, with code=%d", stat);
            break;
        case SGX_ERROR_OUT_OF_MEMORY:
            printf("SGX_ERROR_OUT_OF_MEMORY, with code=%d", stat);
            break;
        case SGX_ERROR_OUT_OF_EPC:
            printf("SGX_ERROR_OUT_OF_EPC, with code=%d", stat);
            break;
        case SGX_ERROR_UNEXPECTED:
            printf("SGX_ERROR_UNEXPECTED, with code=%d", stat);
            break;
        case SGX_ERROR_AE_SESSION_INVALID:
            printf("SGX_ERROR_AE_SESSION_INVALID, with code=%d", stat);
            break;
        case SGX_ERROR_SERVICE_UNAVAILABLE:
            printf("SGX_ERROR_SERVICE_UNAVAILABLE, with code=%d", stat);
            break;
        case SGX_ERROR_SERVICE_TIMEOUT:
            printf("SGX_ERROR_SERVICE_TIMEOUT, with code=%d", stat);
            break;
        case SGX_ERROR_BUSY:
            printf("SGX_ERROR_BUSY, with code=%d", stat);
            break;
        case SGX_ERROR_NETWORK_FAILURE:
            printf("SGX_ERROR_NETWORK_FAILURE, with code=%d", stat);
            break;
        case SGX_ERROR_UPDATE_NEEDED:
            printf("SGX_ERROR_UPDATE_NEEDED, with code=%d", stat);
            break;
        default:
            printf("Unknown error, with code=%d", stat);
    }
    return;
}


/****************************************************
 * BENCHMARKING
 ****************************************************/

void benchmarkTimeAES(size_t processTime){
    TIME_AES_METHOD += processTime;
    COUNT_AES_METHOD += 1;
}

void benchmarkTimeAutomation(size_t processTime){
    TIME_AUTOMATION += processTime;
    COUNT_AUTOMATION += 1;
}

void benchmarkTimeRuleConflict(size_t processTime){
    TIME_RULE_CONFLICT_DETECTION += processTime;
    COUNT_RULE_CONFLICT_DETECTION += 1;
}

void printBenchmarkResults(){
    //printf("### Benchmark Results:");
    if (COUNT_AES_METHOD != 0) printf("AVG Encryption/Decryption Time: %d", TIME_AES_METHOD/COUNT_AES_METHOD);
    if (COUNT_AUTOMATION != 0) printf("AVG Rule Automation Time: %d", TIME_AUTOMATION/COUNT_AUTOMATION);
    if (COUNT_RULE_CONFLICT_DETECTION != 0) printf("AVG Rule Conflict Detection Time: %d", TIME_RULE_CONFLICT_DETECTION/COUNT_RULE_CONFLICT_DETECTION);
}

