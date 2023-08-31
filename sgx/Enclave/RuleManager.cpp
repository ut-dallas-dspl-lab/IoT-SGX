#include "RuleManager.h"
#include "Enclave.h"
#include "Enclave_t.h"
#include "RuleParser.h"
#include "analytics_utils.h"
#include "RuleConflictDetectionManager.h"
#include "EnclaveDatabaseManager.h"
#include "ObliviousOperationManager.h"


/******************************************************/
/* Rule Parsing */
/******************************************************/

/*
 * startParsingRule:
 *  Parse the decrypted rule string and make a Struct Rule
 *  @params: decrypted rule string, Struct Rule
 *  returns: true if successful, else false
 */
bool startParsingRule(char *ruleString, Rule *myRule){
    //if (IS_DEBUG) printf("RuleManager:: #startParsingRule");
    RuleType ruleType = parseRuleTypeAction(ruleString);

    bool isValidRuleType = false, isSuccess = false;
    isValidRuleType = obliviousSelectEq(ruleType, IF, true, false);
    isSuccess = isValidRuleType && parseRule(ruleString, ruleType, myRule);

    if (IS_DEBUG){
        if (!isValidRuleType) printf("RuleManager:: Unknown Rule type! ");
        if (!isSuccess) printf("RuleManager:: Rule parsing failed! ");
    }
    return isSuccess;
}


/******************************************************/
/* Rule Operation */
/******************************************************/

/*
 * startRuleConflictDetection:
 *  returns false if there's no conflict, else return true.
 */
bool startRuleConflictDetection(Rule *myRule){
    if (IS_DEBUG) printf("RuleManager:: #startRuleConflictDetection");
    bool ret = detectRuleConflicts(myRule);
    return ret;
}

/*
 * checkRuleSatisfiability:
 *  if the device event matches the trigger properties of a Rule, then the rule is satisfied
 *  @params: an incoming device event, a Rule fetched from DB
 *  returns true if the Rule is satisfied, else false.
 */
bool checkRuleSatisfiability(DeviceEvent *myEvent, Rule *myRule){
    bool didDeviceAttributeMatch = false;
    didDeviceAttributeMatch = obliviousAndOperation((obliviousStringCompare(myEvent->deviceID, myRule->trigger->deviceID)), (obliviousStringCompare(myEvent->attribute, myRule->trigger->attribute)));
    if (IS_DEBUG && !didDeviceAttributeMatch) printf("RuleManager:: Failed! Devices or attributes do not match!");

    bool isValueTypeString = false, isValueTypeNumber = false;

    isValueTypeString = obliviousSelectEq(myEvent->valueType, STRING);
    isValueTypeString = obliviousAndOperation(obliviousSelectEq(myRule->trigger->valueType, STRING), isValueTypeString);
    isValueTypeNumber = obliviousSelectEq(myEvent->valueType, NUMBER);
    isValueTypeNumber = obliviousAndOperation(obliviousSelectEq(myRule->trigger->valueType, NUMBER), isValueTypeNumber);
    if (IS_DEBUG && !isValueTypeString && !isValueTypeNumber) printf("RuleManager:: Failed! Value types do not match!");

    isValueTypeString = obliviousAndOperation(didDeviceAttributeMatch, isValueTypeString);
    isValueTypeNumber = obliviousAndOperation(didDeviceAttributeMatch, isValueTypeNumber);

    bool is_satisfied = false, is_satisfied_str = false, is_satisfied_num_eq = false, is_satisfied_num_gt = false, is_satisfied_num_lt = false;
    float event_value = myEvent->value, rule_tr_value = myRule->trigger->value;

    is_satisfied_str = obliviousStringCompare(myEvent->valueString, myRule->trigger->valueString);
    is_satisfied_str = obliviousAndOperation(is_satisfied_str, isValueTypeString);


    is_satisfied_num_eq = obliviousAndOperation(obliviousSelectEq(myRule->trigger->operatorType, EQ), obliviousSelectEq(event_value, rule_tr_value));
    is_satisfied_num_eq = obliviousAndOperation(is_satisfied_num_eq, isValueTypeNumber);

    is_satisfied_num_gt = obliviousAndOperation(obliviousSelectEq(myRule->trigger->operatorType, GT), obliviousSelectGt(event_value, rule_tr_value, true, false));
    is_satisfied_num_gt = obliviousAndOperation(is_satisfied_num_gt, isValueTypeNumber);

    is_satisfied_num_lt = obliviousAndOperation(obliviousSelectEq(myRule->trigger->operatorType, LT), obliviousSelectLt(event_value, rule_tr_value, true, false));
    is_satisfied_num_lt = obliviousAndOperation(is_satisfied_num_lt, isValueTypeNumber);

    is_satisfied = is_satisfied_str || is_satisfied_num_eq || is_satisfied_num_gt || is_satisfied_num_lt;

    if (IS_DEBUG && is_satisfied) printf("RuleManager:: Success! Device event satisfies the rule!");
    if (IS_DEBUG && !is_satisfied) printf("RuleManager:: Failed! Values do not match!");

    return is_satisfied;
}

/*
 * startRuleAutomation:
 *  Check if an incoming device event satisfies any rule stored in the DB.
 *  @params: device event
 *  returns: true if any Rule is satisfied, else returns false.
 */
bool startRuleAutomation(DeviceEvent *myEvent){
    if (IS_DEBUG) printf("\nStarting Rule Automation ...");

    std::vector<Rule*> ruleset;
    bool isSuccess = false;

    isSuccess = retrieveRulesFromDB(ruleset, 0, myEvent->deviceID, "", BY_TRIGGER_DEVICE_ID);

    int loop_num = ruleset.size();
    TOTAL_RETRIEVED_RULES += ruleset.size();
    for (int i = 0; i < loop_num; ++i){
        if (IS_DEBUG) printf("\nRuleAutomation: Rule#%d",i+1);

        bool ret = obliviousAndOperation(isSuccess, checkRuleSatisfiability(myEvent, ruleset[i]));

        //Assuming dummy rules already contain dummy device id
        sendRuleCommands(ruleset[i], ret);
    }

    if (IS_DEBUG){
        if (!isSuccess) printf("RuleManager:: Rule retrieval failed!");
    }
    ruleset.clear();

    return isSuccess;
}

/*
 * sendRuleCommands:
 *  sends action-commands to respective devices according to the Action part of the Rule
 *  @params: a Rule, a boolean indicating whether the rule is satisfied
 *  returns: true if command sent successfully, else false
 */
bool sendRuleCommands(Rule *myRule, bool isRuleSatisfied){
    TOTAL_DEVICE_COMMANDS += 1;
    if (IS_DEBUG) printf("isRuleSatisfied = %d", isRuleSatisfied);

    /* check if there is any 'else' response in the Rule */
    bool isEmptyResponse = obliviousSelectEq(isRuleSatisfied, 0) && obliviousSelectEq(myRule->isElseExist, 0);
    if (isEmptyResponse) return isRuleSatisfied;

    Message *response = (Message*) malloc(sizeof(Message));
    response->isEncrypted = IS_ENCRYPTION_ENABLED;

    response->address = obliviousSelectEq(isRuleSatisfied, 1) ? myRule->action->deviceID : myRule->actionElse->deviceID;
    char *responseCommand = obliviousSelectEq(isRuleSatisfied, 1) ? myRule->responseCommand : myRule->responseCommandElse;
    int len = obliviousStringLength(responseCommand);
    if (IS_DEBUG) printf("responseCommand = %s", responseCommand);

    response->textLength = len+SGX_AESGCM_MAC_SIZE;
    response->text = (char *) malloc(sizeof(char) * (response->textLength+1));
    response->tag = NULL;
    bool isEncryptionSuccess = encryptMessage_AES_GCM(responseCommand, len, response->text, response->textLength);

    size_t isSuccess = 0;
    if (obliviousSelectEq(isEncryptionSuccess,1)) ocall_send_rule_commands_mqtt(&isSuccess, response->address, response->text, response->textLength); /* pass the response to the REE via ocall */
    if (IS_DEBUG) isSuccess ? printf("RuleManager:: Successfully sent rule command to device!") : printf("RuleManager:: Failed to send rule command to device!");

    deleteMessage(&response);
    return isSuccess;
}


/******************************************************/
/* Dummy Rule Generation */
/******************************************************/

/**
 * Create a number of dummy rules based on a generated Random number and store the rules in DB. Limit 10.
 * @param ruleString
 * @param myRule
 */
bool createDummyRules(char *ruleString, Rule *myRule){
    int random = generateRandomNumber(10);
    char *temp = myRule->ruleID;

    size_t ts = ocallGetCurrentTime(MILLI);

    for (int i = 0; i < random; ++i) {
        std::string dummy_str("DummyRule");
        std::string numStr = std::to_string(ts + i);
        dummy_str.append(numStr);

        int len = strlen(myRule->ruleID) + dummy_str.length() + 1;
        char *dummyRuleID = (char *) malloc(sizeof(char) * (len+1));
        memcpy(dummyRuleID, myRule->ruleID, strlen(myRule->ruleID));
        memcpy(dummyRuleID + strlen(myRule->ruleID), dummy_str.c_str(), dummy_str.length());
        dummyRuleID[len] = '\0';

        myRule->ruleID = dummyRuleID;
        storeRuleInDB(ruleString, myRule);

        free(dummyRuleID);
    }
    myRule->ruleID = temp;
    return true;
}

/**
 * Create one dummy rule by copying from an existing rule
 * @param myRule
 * @param dummyRule
 */
bool createDummyRule(Rule *myRule, Rule *dummyRule){
    copyRule(&myRule, &dummyRule);

    int len = strlen(dummyRule->ruleID) + strlen("dummy");
    char *dummyRuleID = (char *) malloc(sizeof(char) * (len+1));
    memcpy(dummyRuleID, dummyRule->ruleID, strlen(dummyRule->ruleID));
    memcpy(dummyRuleID + strlen(dummyRule->ruleID), "dummy", strlen("dummy"));
    dummyRuleID[len] = '\0';

    if(dummyRule->ruleID != NULL) free(dummyRule->ruleID);
    dummyRule->ruleID = dummyRuleID;

    return true;
}