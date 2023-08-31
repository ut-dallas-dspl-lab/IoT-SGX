#ifndef IOTENCLAVE_MONGOMANAGER_H
#define IOTENCLAVE_MONGOMANAGER_H

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <string>
#include <vector>

#include "sgx_urts.h"
#include "Enclave_u.h"

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

enum DBKeyType {DB_DEVICE_ID_TRIGGER, DB_DEVICE_ID_ACTION, DB_RULE_ID, DB_ATTRIBUTE, DB_VALUE_TYPE, DB_DATA, DB_IS_ENC, DB_UNKNOWN};


class MongoManager {
private:
    std::string url;
    std::string db;
    std::string coll;
    mongocxx::collection myCollection;

public:
    MongoManager(std::string urlName, std::string dbName, std::string collectionName);
    int initConnection();
    std::string getKeyString(DBKeyType key);

    bool insertRule(DatabaseElement *element, int count);
    int retrieveRuleCount(DatabaseElement *element);
    bool retrieveRuleInfo(DatabaseElement *element, size_t numRules);
    bool retrieveRule(DatabaseElement *element, size_t numRules);

};



#endif //IOTENCLAVE_MONGOMANAGER_H
