#include <string>
#include "EnclaveHelper.h"

#ifndef IOT_SGX_NEW_RULECONFLICTDETECTIONMANAGER_H
#define IOT_SGX_NEW_RULECONFLICTDETECTIONMANAGER_H

typedef std::pair<float, float> RangePair;
typedef std::pair<std::string, std::string> KeyPair;

bool detectRuleConflicts(Rule *edge);


#endif //IOT_SGX_NEW_RULECONFLICTDETECTIONMANAGER_H
