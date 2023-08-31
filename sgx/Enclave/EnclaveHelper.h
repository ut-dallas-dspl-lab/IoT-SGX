#ifndef IOTENCLAVE_ENCLAVEHELPER_H
#define IOTENCLAVE_ENCLAVEHELPER_H

#include "Enclave.h"
#include "Enclave_t.h"
#include <string>
#include <vector>

extern bool IS_ENCRYPTION_ENABLED;
extern bool IS_DEBUG;
extern bool IS_BENCHMARK;
extern bool IS_CFI_ENABLED;
extern bool IS_DATA_OBLIVIOUSNESS_ENABLED;
extern bool IS_PADDED;

extern int TOTAL_CONFLICTED_RULES;
extern int TOTAL_RETRIEVED_RULES;
extern int TOTAL_DEVICE_COMMANDS;

extern int TIME_AES_METHOD;
extern int COUNT_AES_METHOD;
extern int TIME_RULE_CONFLICT_DETECTION;
extern int COUNT_RULE_CONFLICT_DETECTION;
extern int TIME_AUTOMATION;
extern int COUNT_AUTOMATION;


/* Utility functions */
void toLowerCase(char *str);
int generateRandomNumber(int limit);

/* Timer Helpers */
TimeUnitType getTimeUnitType (char *key);
int getTimeSecond(int h, int m);
int getTimeMinute(int h, int m);

/* Initialize & Free structs */
bool initRule(Rule **myrule);
bool copyRule(Rule **rule, Rule **newRule);
void deleteRule(Rule **myrule);
void deleteDeviceEvent(DeviceEvent **myEvent);
void deleteMessage(Message **msg);
void deleteDbElement(DatabaseElement **dbElement);

/* Methods for Enums */
RuleType getRuleType(char *key);
ValueType getValueType(char *key);
OperatorType getOperatorType (char *key);

/* Printing Info */
void printRuleInfo(Rule *myRule);
void printDeviceEventInfo(DeviceEvent *myEvent);
void printConflictType(ConflictType conflict);

/* Error functions */
void check_error_code(sgx_status_t stat);

/* Benchmark */
void printBenchmarkResults();
void benchmarkTimeAES(size_t processTime);
void benchmarkTimeAutomation(size_t processTime);
void benchmarkTimeRuleConflict(size_t processTime);



#endif //IOTENCLAVE_ENCLAVEHELPER_H
