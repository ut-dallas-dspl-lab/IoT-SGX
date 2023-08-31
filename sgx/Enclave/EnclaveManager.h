#ifndef IOT_SGX_NEW_ENCLAVEMANAGER_H
#define IOT_SGX_NEW_ENCLAVEMANAGER_H

#include "Enclave_t.h"
#include "ControlFlowManager.h"

void setupEnclave();
int setupDeviceInfo(char *info);

int getNumSubscriptionTopics();
int getSubscriptionTopicNames(struct Message *msg, size_t num_topics);

int didReceiveRule(struct Message *msg);
int didReceiveEvent(struct Message *msg);
void didReceiveEventMQTT(struct Message *msg);


#endif //IOT_SGX_NEW_ENCLAVEMANAGER_H
