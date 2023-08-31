#include "RuleConflictDetectionManager.h"
//#include <vector>
//#include <map>
//#include <algorithm>
#include "RuleManager.h"
#include "EnclaveDatabaseManager.h"

#define MAX 100000
#define MIN -100000


RangePair getRangePair(float value, enum OperatorType op){
    if (op == GT){
        return std::make_pair(value+1,MAX);
    } else if (op == LT){
        return std::make_pair(MIN,value-1);
    } else if (op == GTE){
        return std::make_pair(value,MAX);
    } else if (op == LTE){
        return std::make_pair(MIN,value);
    } else{
        return std::make_pair(value,value);
    }
}

bool isOverlap(float value1, enum OperatorType op1, float value2, enum OperatorType op2){
    RangePair p1 = getRangePair(value1, op1);
    RangePair p2 = getRangePair(value2, op2);
    return std::max(p1.first, p2.first) <= std::min(p1.second, p2.second);
}

bool isStringValue(RuleComponent *item){
    if(item->valueType == STRING)
        return true;
    return false;
}

bool isNumberValue(RuleComponent *item){
    if(item->valueType == NUMBER || item->valueType == INTEGER)
        return true;
    return false;
}

bool isValueEqual(RuleComponent *item1, RuleComponent *item2){
    if(isStringValue(item1) && isStringValue(item2)){
        return strcmp(item1->valueString, item2->valueString) == 0;
    } else if(isNumberValue(item1) && isNumberValue(item2)){
        return item1->value == item2->value;
    } else{
        return false;
    }
}


/******************************************************
 * Rule Conflict Detection
 ******************************************************/

bool checkShadowExecutionConflictBound(Rule *newEdge, Rule *oldEdge){
    if(isStringValue(newEdge->trigger) && isStringValue(oldEdge->trigger)){
        return strcmp(newEdge->trigger->valueString, oldEdge->trigger->valueString) == 0;
    }else if (isNumberValue(newEdge->trigger) && isNumberValue(oldEdge->trigger)){
        return isOverlap(newEdge->trigger->value, newEdge->trigger->operatorType, oldEdge->trigger->value, oldEdge->trigger->operatorType);
    }
    return false;
}

/*
 * Shadow or Execution Conflict detection
 *  Shadow: For Rules X and Y  => Trigger(X) ⊆ Trigger(Y), Actuator(X) = Actuator(Y), Action(X) = Action(Y)
 *  Execution: For Rules X and Y  => Trigger(X) ⊆ Trigger(Y), Actuator(X) = Actuator(Y), Action(X) ≠ Action(Y)
 */
ConflictType checkShadowExecutionConflict(Rule *newEdge, Rule *oldEdge){
    bool isSameAction = isValueEqual(newEdge->action, oldEdge->action);
    if(isSameAction){
        return checkShadowExecutionConflictBound(newEdge, oldEdge)? SHADOW : NONE;
    }else{
        return checkShadowExecutionConflictBound(newEdge, oldEdge)? EXECUTION : NONE;
    }
}

/*
 * Environment Mutual Conflict (continuous toggle) detection
 *  For Rules X and Y  => Trigger(X) ≠ Trigger(Y), Actuator(X) = Actuator(Y),  Action(X) ≠ Action(Y)
 */
ConflictType checkMutualConflict(Rule *newEdge, Rule *oldEdge){
    /*
    if(isStringValue(newEdge->action)){
        printf("newEdge->action->valueString=%s, oldEdge->action->valueString=%s",newEdge->action->valueString, oldEdge->action->valueString);
        if(strcmp(newEdge->action->valueString, oldEdge->action->valueString) != 0)
            return MUTUAL;
    }else{
        printf("newEdge->action->value=%f, oldEdge->action->value=%f",newEdge->action->value, oldEdge->action->value);
        if(newEdge->action->value != oldEdge->action->value)
            return MUTUAL;
    }*/
    if(!isValueEqual(newEdge->action, oldEdge->action)){
        return MUTUAL;
    }
    return NONE;
}

/*
 * Dependence conflict detection
 *  For Rules X and Y  => Action(X) ➜ Trigger(Y), Action(Y) ➜ Trigger(X)
 */
ConflictType checkDependenceConflict(Rule *newEdge, Rule *oldEdge){
    if(!isValueEqual(newEdge->action, oldEdge->trigger) && !isValueEqual(newEdge->trigger, oldEdge->action))
        return NONE;
    return DEPENDENCE;
}

/*
 * Detection of Chaining of the rules with the given rule
 *  Forward Chaining: new Rule's action activates an existing Rule's trigger
 *  Backward Chaining: new Rule's trigger will be activated by an existing Rule's action
 */
ConflictType checkChainingConflict(Rule *newEdge, Rule *oldEdge, bool isBackwardChaining){
    if(isBackwardChaining){
        if(isValueEqual(newEdge->trigger, oldEdge->action))
            return CHAIN;
    }else{
        if(isValueEqual(newEdge->action, oldEdge->trigger))
            return CHAIN;
    }
    return NONE;
}

/*
 * detectRuleConflicts:
 *  for a given rule, detects conflicts with existing rules in DB
 *  @params: the given rule
 *  returns: true if there exist a conflict, else false
 */
bool detectRuleConflicts(Rule *edge){
    ConflictType result = NONE;
    std::vector<Rule*> ruleset;
    /* check shadow, execution, and mutual conflicts: retrieve Rules from DB where action device match with the given Rule's action device */
    if(retrieveRulesFromDB(ruleset, 0, edge->action->deviceID, NULL, BY_ACTION_DEVICE_ID)){
        if (IS_DEBUG) printf("RuleConflictDetectionManager:: Retrieved rule!, size=%d", ruleset.size());
        for (int i = 0; i < ruleset.size(); ++i) {
            if (strstr(ruleset[i]->ruleID, "dummy") != NULL) continue; //leave the dummy rules
            if (strcmp(edge->trigger->deviceID, ruleset[i]->trigger->deviceID) == 0){
                /* shadow or execution */
                result =  checkShadowExecutionConflict(edge, ruleset[i]);
                if(result != NONE) break;
            }else{
                /* mutual */
                result = checkMutualConflict(edge, ruleset[i]);
                if(result != NONE) break;
            }
        }
        for (int i = 0; i < ruleset.size(); ++i) deleteRule(&ruleset[i]);
        ruleset.clear();
        if (result != NONE){
            printConflictType(result);
            return true;
        }
    }else{
        printf("RuleConflictDetectionManager:: Rule retrieval failed!");
    }

    /* check dependence and forward chaining conflicts: retrieve Rules from DB where trigger device match with the given Rule's action device */
    if(retrieveRulesFromDB(ruleset, 0, edge->action->deviceID, NULL, BY_TRIGGER_DEVICE_ID)){
        if (IS_DEBUG) printf("RuleConflictDetectionManager:: Retrieved rule!, size=%d", ruleset.size());
        for (int i = 0; i < ruleset.size(); ++i) {
            if (strstr(ruleset[i]->ruleID, "dummy") != NULL) continue; //leave the dummy rules
            if (strcmp(edge->trigger->deviceID, ruleset[i]->action->deviceID) == 0){
                /* dependence */
                result =  checkDependenceConflict(edge, ruleset[i]);
                if(result != NONE) break;
            }else{
                /* forward chaining */
                result = checkChainingConflict(edge, ruleset[i], false);
                if(result != NONE) break;
            }
        }
        for (int i = 0; i < ruleset.size(); ++i) deleteRule(&ruleset[i]);
        ruleset.clear();
        if (result != NONE){
            printConflictType(result);
            return true;
        }
    }else{
        printf("RuleConflictDetectionManager:: Rule retrieval failed!");
    }

    /* check backward chaining conflicts: retrieve Rules from DB where action device match with the given Rule's trigger device */
    if(retrieveRulesFromDB(ruleset, 0, edge->trigger->deviceID, NULL, BY_ACTION_DEVICE_ID)){
        if (IS_DEBUG) printf("RuleConflictDetectionManager:: Retrieved rule!, size=%d", ruleset.size());
        for (int i = 0; i < ruleset.size(); ++i) {
            if (strstr(ruleset[i]->ruleID, "dummy") != NULL) continue; //leave the dummy rules
            /* backward chaining */
            result = checkChainingConflict(edge, ruleset[i], true);
            if(result != NONE) break;
        }
        for (int i = 0; i < ruleset.size(); ++i) deleteRule(&ruleset[i]);
        ruleset.clear();
        if (result != NONE){
            printConflictType(result);
            return true;
        }
    }else{
        printf("RuleConflictDetectionManager:: Rule retrieval failed!");
    }

    return result == NONE? false:true;
}



