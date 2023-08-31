#include <string>
#include <map>
#include <vector>
#include <queue>
#include "EnclaveHelper.h"
#include "ControlFlowManager.h"

#ifndef IOTENCLAVE_RULEMANAGER_H
#define IOTENCLAVE_RULEMANAGER_H

bool startParsingRule(char *ruleString, Rule *myRule);

bool startRuleAutomation(DeviceEvent *myEvent);

bool sendRuleCommands(Rule *myRule, bool isRuleSatisfied);

bool startRuleConflictDetection(Rule *myRule);

bool createDummyRules(char *ruleString, Rule *myRule);
bool createDummyRule(Rule *myRule, Rule *dummyRule);

#endif //IOTENCLAVE_RULEMANAGER_H
