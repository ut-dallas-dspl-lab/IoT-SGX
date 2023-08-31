#include "ControlFlowManager.h"
#include "Enclave.h"
#include "Enclave_t.h"

CFITaskType calledFunction = TaskType_None;
CFITaskType returnFunction = TaskType_None;


/*
 * Check Control Flow Integrity
 *  @params:
 *  @return:
 */
void init_cfi(CFITaskType callFunc, CFITaskType retFunc){
    calledFunction = callFunc;
    returnFunction = retFunc;
}

void cfi_set_callee(CFITaskType callFunc){
    calledFunction = callFunc;
}

void cfi_set_return(CFITaskType retFunc){
    returnFunction = retFunc;
}

void cfi_check_callee(CFITaskType callFunc){
    if(calledFunction != callFunc){
        printf("CFI Call Error! CFITaskType=%d", callFunc);
        //exit(0);
    }
}

void cfi_check_return(CFITaskType retFunc){
    if(returnFunction != retFunc){
        printf("CFI Return Error! CFITaskType=%d", retFunc);
        //exit(0);
    }
}

void dummy_cfi_func(CFITaskType retTaskType){
    cfi_check_callee(TaskType_Dummy);
    printf("called dummy function...");
    cfi_set_return(retTaskType);
}