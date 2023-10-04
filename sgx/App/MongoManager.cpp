#include "MongoManager.h"
#include "JSONParser.h"
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/error_code.hpp>
#include <mongocxx/exception/logic_error.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/exception/query_exception.hpp>
#include <mongocxx/exception/server_error_code.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/stdx/make_unique.hpp>


/* Constructor: Init MongoManager instance
 * @param: URI
 * @param: Database name
 * @param: Collection name
 */
MongoManager::MongoManager(std::string urlName, std::string dbName, std::string collectionName) {
    url = urlName;
    db = dbName;
    coll = collectionName;
    mongocxx::instance instance{}; // This should be done only once.
}

/* Establish connection */
int MongoManager::initConnection() {
    //mongocxx::uri uri("mongodb://localhost:27017");
    mongocxx::uri uri(url);
    mongocxx::client client(uri);

    mongocxx::database myDb = client[db];

    mongocxx::collection col = myDb[coll];

    return 0;
}


std::string MongoManager::getKeyString(DBKeyType key){
    switch (key){
        case DB_RULE_ID:
            return "rule_id";
        case DB_DEVICE_ID_TRIGGER:
            return "device_id_trigger";
        case DB_DEVICE_ID_ACTION:
            return "device_id_action";
        case DB_ATTRIBUTE:
            return "value";
        case DB_VALUE_TYPE:
            return "value_type";
        case DB_DATA:
            return "data";
        case DB_IS_ENC:
            return "is_enc";
        default:
            return "";
    }
}

/*
 * Insert Rules to MongoDB collection
 * @param element: DatabaseElement struct containing encrypted rules and other info
 * @param count: total number of rules to insert
 */
bool MongoManager::insertRule(DatabaseElement *element, int count){
    mongocxx::uri uri(url);
    mongocxx::client client(uri);
    mongocxx::database myDb = client[db];
    myCollection = myDb[coll];

    printf("MongoManager:: #insertRule\n");
    std::vector<bsoncxx::document::value> documents;

    for (int i = 0; i < count; i++) {
        auto builder = bsoncxx::builder::stream::document{};
        std::string data;

        if(element->data->isEncrypted){
            //data = make_json_from_message(element->data);
            data = encode_message(element->data->text, element->data->textLength);
        } else{
            data = std::string(element->data->text);
        }
        if(element->valueType == STRING){
            bsoncxx::document::value doc_value = builder
                    << getKeyString(DB_RULE_ID) << element->ruleID
                    << getKeyString(DB_DEVICE_ID_TRIGGER) << element->deviceID
                    << getKeyString(DB_ATTRIBUTE) << element->attribute
                    << getKeyString(DB_VALUE_TYPE) << element->valueType
                    << getKeyString(DB_DEVICE_ID_ACTION) << element->deviceIDAction
                    << getKeyString(DB_IS_ENC) << static_cast<int>(element->data->isEncrypted)
                    << getKeyString(DB_DATA) << data
                    << finalize;
            documents.push_back(doc_value);
        }else{
            bsoncxx::document::value doc_value = builder
                    << getKeyString(DB_RULE_ID) << element->ruleID
                    << getKeyString(DB_DEVICE_ID_TRIGGER) << element->deviceID
                    << getKeyString(DB_ATTRIBUTE) << atof(element->attribute)
                    << getKeyString(DB_VALUE_TYPE) << element->valueType
                    << getKeyString(DB_DEVICE_ID_ACTION) << element->deviceIDAction
                    << getKeyString(DB_IS_ENC) << static_cast<int>(element->data->isEncrypted)
                    << getKeyString(DB_DATA) << data
                    << finalize;
            documents.push_back(doc_value);
        }

        if(i != count-1) element++;
    }

    try {
        myCollection.insert_many(documents);
    }catch (const mongocxx::bulk_write_exception& e){
        std::cout << "MongoManager:: insert_many operation throws: " << e.what() << std::endl;
        if (e.raw_server_error()) {
            std::cout << "MongoManager:: Raw server error:" << std::endl;
            std::cout << bsoncxx::to_json(*(e.raw_server_error())) << std::endl;
        }
        return false;
    }
    printf(" MongoManager:: Insertion done!!!\n");
    return true;
}

/*
 * Retrieve the number of rules from MongoDB collection following a query
 * @param element: DatabaseElement struct containing the query
 */
int MongoManager::retrieveRuleCount(DatabaseElement *element){
    //printf("MongoManager:: retrieveRuleCount!");
    mongocxx::uri uri(url);
    mongocxx::client client(uri);
    mongocxx::database myDb = client[db];
    myCollection = myDb[coll];

    //printf("rid=%s, did=%s, adid=%s, attr=%s\n", element->ruleID, element->deviceID, element->deviceIDAction, element->attribute);

    int count = 0;
    auto builder = bsoncxx::builder::stream::document{};
    bsoncxx::document::value query({});
    switch (element->queryType){
        case BY_RULE:{
            //count = myCollection.count_documents(document {} << getKeyString(DB_RULE_ID) << element->ruleID << finalize);
            query = builder << getKeyString(DB_RULE_ID) << element->ruleID << finalize;
            break;
        }
        case BY_TRIGGER_DEVICE_ID:{
            //count = myCollection.count_documents(document{} << getKeyString(DB_DEVICE_ID_TRIGGER) << element->deviceID << finalize);
            query = builder << getKeyString(DB_DEVICE_ID_TRIGGER) << element->deviceID << finalize;
            break;
        }
        case BY_TRIGGER_DEVICE_ID_ATTR:{
            //count = myCollection.count_documents(document{} << getKeyString(DB_DEVICE_ID_TRIGGER) << element->deviceID << getKeyString(DB_ATTRIBUTE) << element->attribute << finalize);
            query = builder << getKeyString(DB_DEVICE_ID_TRIGGER) << element->deviceID << getKeyString(DB_ATTRIBUTE) << element->attribute << finalize;
            break;
        }
        case BY_ACTION_DEVICE_ID:{
            //count = myCollection.count_documents(document{} << getKeyString(DB_DEVICE_ID_ACTION) << element->deviceIDAction << finalize);
            query = builder << getKeyString(DB_DEVICE_ID_ACTION) << element->deviceIDAction << finalize;
            break;
        }
        case BY_TRIGGER_ACTION_DEVICE_ID:{
            //count = myCollection.count_documents(document{} << getKeyString(DB_DEVICE_ID_TRIGGER) << element->deviceID << getKeyString(DB_DEVICE_ID_ACTION) << element->deviceIDAction << finalize);
            query = builder << getKeyString(DB_DEVICE_ID_TRIGGER) << element->deviceID << getKeyString(DB_DEVICE_ID_ACTION) << element->deviceIDAction << finalize;
            break;
        }
        case ALL_QUERY:{
            //count = myCollection.count_documents({});
            query = builder << finalize;
            break;
        }
        default: {
            printf("MongoManager:: Unknown DB Query Type!");
            return -1;
        }
    }

    try {
        count = myCollection.count_documents(query.view());
    } catch (const mongocxx::query_exception& e){
        std::cout << "MongoManager:: count_documents operation throws: " << e.what() << std::endl;
        if (e.raw_server_error()) {
            std::cout << "MongoManager:: Raw server error:" << std::endl;
            std::cout << bsoncxx::to_json(*(e.raw_server_error())) << std::endl;
        }
        return -1;
    }
    //printf("MongoManager:: retrieveRuleCount=%d\n",count);
    return count;
}

/*
 * Retrieve rule info (e.g., count, rule size) from MongoDB collection following a query
 * @param element: DatabaseElement struct that contains the query and stores the info after fetching from DB
 * @param count: total number of rules from retrieveRuleCount()
 */
bool MongoManager::retrieveRuleInfo(DatabaseElement *element, size_t numRules){
    mongocxx::uri uri(url);
    mongocxx::client client(uri);
    mongocxx::database myDb = client[db];
    myCollection = myDb[coll];

    auto builder = bsoncxx::builder::stream::document{};
    bsoncxx::document::value query({});
    switch (element->queryType){
        case BY_RULE:{
            query = builder << getKeyString(DB_RULE_ID) << element->ruleID << finalize;
            break;
        }
        case BY_TRIGGER_DEVICE_ID:{
            query = builder << getKeyString(DB_DEVICE_ID_TRIGGER) << element->deviceID << finalize;
            break;
        }
        case BY_TRIGGER_DEVICE_ID_ATTR:{
            query = builder << getKeyString(DB_DEVICE_ID_TRIGGER) << element->deviceID << getKeyString(DB_ATTRIBUTE) << element->attribute << finalize;
            break;
        }
        case BY_ACTION_DEVICE_ID:{
            query = builder << getKeyString(DB_DEVICE_ID_ACTION) << element->deviceIDAction << finalize;
            break;
        }
        case BY_TRIGGER_ACTION_DEVICE_ID:{
            query = builder << getKeyString(DB_DEVICE_ID_TRIGGER) << element->deviceID << getKeyString(DB_DEVICE_ID_ACTION) << element->deviceIDAction << finalize;
            break;
        }
        case ALL_QUERY:{
            query = builder << finalize;
            break;
        }
        default: {
            printf("MongoManager:: Unknown DB Query Type!");
            return false;
        }
    }

    int count = 0;
    bool isSuccess = true;
    try {
        mongocxx::cursor cursor = myCollection.find(query.view());
        for(auto doc : cursor) {
            //std::cout << "MongoManager:: " << bsoncxx::to_json(doc) << std::endl;
            std::string data;
            int is_encrypted = 0;

            bsoncxx::document::element db_element_encryption{doc[getKeyString(DB_IS_ENC)]};
            if (db_element_encryption) {
                is_encrypted = db_element_encryption.get_int32();
            } else{
                isSuccess = false;
                break;
            }

            bsoncxx::document::element db_element_data{doc[getKeyString(DB_DATA)]};
            if (db_element_data) {
                data = db_element_data.get_utf8().value.to_string();
            } else{
                isSuccess = false;
                break;
            }

            element->data->isEncrypted = is_encrypted;
            if (is_encrypted){
                element->data->textLength = get_decoded_message_length((char *)data.c_str());
            }else{
                element->data->textLength = data.length();
            }
            //printf("MongoManager::retrieveRuleInfo: count=%d, data_len=%d\n", count, element->data->textLength);
            count++;
            if(count != numRules) element->data++;
        }
    } catch (const mongocxx::query_exception& e){
        std::cout << "MongoManager:: find operation throws: " << e.what() << std::endl;
        if (e.raw_server_error()) {
            std::cout << "MongoManager:: Raw server error:" << std::endl;
            std::cout << bsoncxx::to_json(*(e.raw_server_error())) << std::endl;
        }
        return false;
    }

    if(!isSuccess || count==0) printf("MongoManager:: Failed to retrieve rule...\n");
    return isSuccess;
}

/*
 * Retrieve encrypted rules from MongoDB collection following a query
 * @param element: DatabaseElement struct that contains the query and stores the rules after fetching from DB
 * @param count: total number of rules from retrieveRuleCount()
 */
bool MongoManager::retrieveRule(DatabaseElement *element, size_t numRules){
    mongocxx::uri uri(url);
    mongocxx::client client(uri);
    mongocxx::database myDb = client[db];
    myCollection = myDb[coll];

    auto builder = bsoncxx::builder::stream::document{};
    bsoncxx::document::value query({});
    switch (element->queryType){
        case BY_RULE:{
            query = builder << getKeyString(DB_RULE_ID) << element->ruleID << finalize;
            break;
        }
        case BY_TRIGGER_DEVICE_ID:{
            query = builder << getKeyString(DB_DEVICE_ID_TRIGGER) << element->deviceID << finalize;
            break;
        }
        case BY_TRIGGER_DEVICE_ID_ATTR:{
            query = builder << getKeyString(DB_DEVICE_ID_TRIGGER) << element->deviceID << getKeyString(DB_ATTRIBUTE) << element->attribute << finalize;
            break;
        }
        case BY_ACTION_DEVICE_ID:{
            query = builder << getKeyString(DB_DEVICE_ID_ACTION) << element->deviceIDAction << finalize;
            break;
        }
        case BY_TRIGGER_ACTION_DEVICE_ID:{
            query = builder << getKeyString(DB_DEVICE_ID_TRIGGER) << element->deviceID << getKeyString(DB_DEVICE_ID_ACTION) << element->deviceIDAction << finalize;
            break;
        }
        case ALL_QUERY:{
            query = builder << finalize;
            break;
        }
        default: {
            printf("MongoManager:: Unknown DB Query Type!");
            return false;
        }
    }

    bool isSuccess = true;
    int count = 0;

    try {
        mongocxx::cursor cursor = myCollection.find(query.view());
        for(auto doc : cursor) {
            //std::cout << "MongoManager:: " << bsoncxx::to_json(doc) << std::endl;
            std::string data;
            int is_encrypted = 0;

            bsoncxx::document::element db_element_encryption{doc[getKeyString(DB_IS_ENC)]};
            if (db_element_encryption) {
                is_encrypted = db_element_encryption.get_int32();
            } else{
                isSuccess = false;
                break;
            }

            bsoncxx::document::element db_element_data{doc[getKeyString(DB_DATA)]};
            if (db_element_data) {
                data = db_element_data.get_utf8().value.to_string();
            } else{
                isSuccess = false;
                break;
            }

            element->data->isEncrypted = is_encrypted;
            element->data->textLength = element->data->isEncrypted ? get_decoded_message_length((char *)data.c_str()) : data.length();
            element->data->text = (char*) malloc(element->data->textLength * sizeof(char));
            if(is_encrypted == 1){
                decode_message((char *)data.c_str(), element->data->text);
            }else{
                memcpy(element->data->text, (char *)data.c_str(), element->data->textLength);
                element->data->text[element->data->textLength] = '\0';
            }
            //printf("MongoManager:: data_len = %d\n", element->data->textLength);
            count++;
            if(count != numRules) element->data++;
        }
    } catch (const mongocxx::query_exception& e){
        std::cout << "MongoManager:: find operation throws: " << e.what() << std::endl;
        if (e.raw_server_error()) {
            std::cout << "MongoManager:: Raw server error:" << std::endl;
            std::cout << bsoncxx::to_json(*(e.raw_server_error())) << std::endl;
        }
        return false;
    }

    if(!isSuccess || count==0) printf("MongoManager:: Failed to retrieve rule...\n");
    return isSuccess;
}
