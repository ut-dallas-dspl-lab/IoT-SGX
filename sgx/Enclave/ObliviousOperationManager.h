#include "Enclave.h"
#include "Enclave_t.h"

#ifndef IOT_SGX_NEW_OBLIVIOUSOPERATIONMANAGER_H
#define IOT_SGX_NEW_OBLIVIOUSOPERATIONMANAGER_H

int obliviousSelectEq(int val1, int val2, int out1, int out2);
int obliviousSelectEq(float val1, float val2, int out1, int out2);
int obliviousSelectGt(float val1, float val2, int out1, int out2);
int obliviousSelectLt(float val1, float val2, int out1, int out2);

int obliviousStringCompare(char *s1, char *s2);
int obliviousStringLength(char *s);

uint64_t __attribute__((noinline)) obliviousAndOperation(uint64_t input1, uint64_t input2);
uint64_t __attribute__((noinline)) obliviousSelectEq(uint64_t input1, uint64_t input2);
size_t o_memcpy(uint32_t pred, void* dst, void* src, size_t size);

#endif //IOT_SGX_NEW_OBLIVIOUSOPERATIONMANAGER_H
