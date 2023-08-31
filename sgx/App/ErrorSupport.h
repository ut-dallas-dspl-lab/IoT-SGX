#ifndef IOTENCLAVE_ERRORSUPPORT_H
#define IOTENCLAVE_ERRORSUPPORT_H

#include "sgx_error.h"

#ifdef __cplusplus
extern "C" {
#endif

void ret_error_support(sgx_status_t ret);


#ifdef __cplusplus
}
#endif

#endif //IOTENCLAVE_ERRORSUPPORT_H
