#include <string>
#include <vector>
#include "EnclaveHelper.h"
#include "ControlFlowManager.h"

#ifndef IOT_SGX_NEW_ENCLAVEDATABASEMANAGER_H
#define IOT_SGX_NEW_ENCLAVEDATABASEMANAGER_H

bool storeRuleInDB(char *ruleString, Rule *rule);
bool retrieveRulesFromDB(std::vector<Rule*>&ruleset, size_t ruleCount, char *primaryKey, char *secondaryKey, DBQueryType queryType);

#endif //IOT_SGX_NEW_ENCLAVEDATABASEMANAGER_H
