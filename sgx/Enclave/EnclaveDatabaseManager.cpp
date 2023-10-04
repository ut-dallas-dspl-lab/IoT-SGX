#include "EnclaveDatabaseManager.h"
#include "analytics_utils.h"
#include "RuleManager.h"
#include "ObliviousOperationManager.h"

void setAttributeValue(RuleComponent *trigger, DatabaseElement *dbElement){
    if(trigger->valueType == STRING){
        dbElement->attribute = trigger->valueString;
        dbElement->valueType = STRING;
    }else if(trigger->valueType == NUMBER){
        dbElement->attribute = (char*)std::to_string(trigger->value).c_str();
        dbElement->valueType = NUMBER;
    }else{
        dbElement->attribute = "";
        dbElement->valueType = UNKNOWN_VALUE;
    }
}

/*
 * storeRuleInDB:
 *  store a rule in the DB after encryption.
 *  @params: rule string, rule struct
 *  returns: true if successful, else false
 */
bool storeRuleInDB(char *ruleString, Rule *myrule){
    if (IS_DEBUG) printf("EnclaveDatabaseManager:: storeRuleInDB!");
    /* initialize struct DatabaseElement */
    DatabaseElement *dbElement = (DatabaseElement*) malloc(sizeof(struct DatabaseElement));
    dbElement->data = (Message*) malloc(sizeof(struct Message));
    size_t len = strlen(ruleString);

    dbElement->ruleID = myrule->ruleID;
    dbElement->deviceID = myrule->trigger->deviceID;
    dbElement->deviceIDAction = myrule->action->deviceID;
    setAttributeValue(myrule->trigger, dbElement);

    dbElement->data->isEncrypted = 1;
    dbElement->data->textLength = len + SGX_AESGCM_MAC_SIZE;
    dbElement->data->text = (char *) malloc(sizeof(char) * (dbElement->data->textLength+1));
    dbElement->data->tag = NULL;
    bool isEncryptionSuccess = encryptMessage_AES_GCM(ruleString, len, dbElement->data->text, dbElement->data->textLength);
    if (!isEncryptionSuccess){
        deleteDbElement(&dbElement);
        return false;
    }

    /* pass the rule to the REE via ocall */
    size_t isSuccess = 0;
    ocall_write_to_file(&isSuccess, dbElement, 1);
    if (IS_DEBUG){
        isSuccess ? printf("EnclaveDatabaseManager:: Rule stored in DB! ") : printf("EnclaveDatabaseManager:: Error! Rule failed to store! ");
    }

    deleteDbElement(&dbElement);

    return isSuccess;
}


/*
 * retrieveRulesFromDB:
 *  retrieve Rules from DB for a query
 *  @params: ruleset = vector to hold the retrieved rules; ruleCount = number of rules to retrieve; primaryKey = secondaryKey = query parameters; queryType = enum to represent the type of query
 *  returns: true if successful, else false
 */
bool retrieveRulesFromDB(std::vector<Rule*>&ruleset, size_t ruleCount, char *primaryKey, char *secondaryKey, DBQueryType queryType) {
    if (IS_DEBUG) printf("EnclaveDatabaseManager:: retrieveRulesFromDB...");

    if (ruleCount <= 0){
        /*  fetch rule count */
        ruleCount = 0;
        ocall_read_rule_count(&ruleCount, primaryKey, secondaryKey, queryType);
        if (ruleCount <= 0) {
            if (IS_DEBUG) printf("EnclaveDatabaseManager:: no rules found in DB...");
            return false;
        }
        if (IS_DEBUG) printf("EnclaveDatabaseManager:: Rule Count = %d", ruleCount);
    }


    /*  fetch rule info such as rule size */
    int *rule_size_list = (int*) malloc(ruleCount * sizeof(int));
    size_t isRetrieved = 0;
    ocall_read_rule_info(&isRetrieved, primaryKey, secondaryKey, queryType, rule_size_list, ruleCount);
    if(isRetrieved == 0){
        if (IS_DEBUG) printf("EnclaveDatabaseManager:: Failed to fetch rule info!");
        /* cleanup */
        free(rule_size_list);
        return false;
    }


    /*  initialize structs Message */
    Message* data = (Message*) malloc(ruleCount * sizeof(Message));
    Message* startPos = data;

    int ruleSize = 0;
    int count = 0;
    for(int i=0; i<ruleCount; i++){
        data->isEncrypted = 1;
        data->textLength = rule_size_list[i];
        data->text = (char *) malloc(sizeof(char) * (data->textLength+1));

        ruleSize += data->textLength;
        count++;
        if(count != ruleCount) data++;
    }
    if (IS_DEBUG) printf("EnclaveDatabaseManager:: Rule Size = %d", ruleSize);


    /* Fetch rules */
    char *data_strings = (char*) malloc((ruleSize+1) * sizeof(char));
    data = startPos;
    ocall_read_rule(&isRetrieved, primaryKey, secondaryKey, queryType, data_strings, ruleSize, ruleCount);

    if(isRetrieved == 0){
        if (IS_DEBUG) printf("EnclaveDatabaseManager:: Failed to fetch rules!");
        /* cleanup */
        count = 0;
        data = startPos;
        for(int i=0; i<ruleCount; i++){
            if(data->text != NULL) free(data->text);
            count++;
            if(count != ruleCount) data++;
        }
        data = startPos;
        free(data_strings);
        free(data);
        free(rule_size_list);
        return false;
    }

    data = startPos;
    int total_len = 0;
    for(int i=0; i<ruleCount; i++){
        memcpy(data->text, data_strings + total_len, data->textLength);
        data->text[data->textLength] = '\0';

        total_len += data->textLength;
        count++;
        if(count != ruleCount) data++;
    }
    free(data_strings);
    free(rule_size_list);


    /* Shuffling */
    data = startPos;
    for(int i=0; i<ruleCount; i++){
        int r = generateRandomNumber(ruleCount);
        Message temp = data[i];
        data[i] = data[r];
        data[r] = temp;
    }
    startPos = data;

    /* Process rules */
    count = 0;
    data = startPos;
    for(int i=0; i<ruleCount; i++){
        if (IS_DEBUG) printf("\n===== EnclaveDatabaseManager:: Rule number: i=%d =====",i+1);

        /* decryption */
        int dec_msg_len = data->textLength - SGX_AESGCM_MAC_SIZE;
        char *ruleString = (char *) malloc((dec_msg_len+1) * sizeof(char));
        bool isDecryptSuccess = decryptMessage_AES_GCM(data->text, data->textLength, ruleString, dec_msg_len);

        if (IS_DEBUG && !isDecryptSuccess){
            printf("EnclaveDatabaseManager:: Error! Failed to decrypt rule!");
        }

        /* parsing */
        struct Rule *myrule;
        initRule(&myrule);

        bool isParsed = isDecryptSuccess && startParsingRule(ruleString, myrule);
        isParsed ? ruleset.push_back(myrule) : deleteRule(&myrule);

        /* clear element heap memory */
        free(ruleString);
        if(data->text != NULL) free(data->text);

        /* go to next element */
        count++;
        if(count != ruleCount) data++;

    }// end for-loop


    /* clean up */
    data = startPos;
    free(data);

    return ruleset.size() > 0 ? true : false;
}