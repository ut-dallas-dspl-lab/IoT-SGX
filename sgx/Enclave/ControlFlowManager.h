#ifndef IOT_SGX_NEW_CONTROLFLOWMANAGER_H
#define IOT_SGX_NEW_CONTROLFLOWMANAGER_H

enum CFITaskType {
    TaskType_None,
    TaskType_Dummy,
    TaskType_ecall_initialize_enclave,
    TaskType_ecall_did_receive_event,
    TaskType_ecall_did_receive_rule,
    TaskType_setupEnclave,
    TaskType_startParsingDeviceEvent,
    TaskType_startParsingRule,
    TaskType_startRuleAutomation,
    TaskType_startRuleConflictDetection,
    TaskType_storeRuleInDB,
    TaskType_retrieveRulesFromDB,
    TaskType_checkRuleSatisfiability,
    TaskType_sendRuleCommands
};

extern CFITaskType calledFunction;
extern CFITaskType returnFunction;

void init_cfi(CFITaskType callFunc, CFITaskType retFunc);
void cfi_set_callee(CFITaskType callFunc);
void cfi_set_return(CFITaskType retFunc);
void cfi_check_callee(CFITaskType callFunc);
void cfi_check_return(CFITaskType retFunc);

void dummy_cfi_func(CFITaskType retTaskType);
#endif //IOT_SGX_NEW_CONTROLFLOWMANAGER_H
