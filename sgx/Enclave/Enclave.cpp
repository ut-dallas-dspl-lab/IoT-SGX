/*
 * Copyright (C) 2011-2018 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "Enclave.h"
#include "Enclave_t.h" /* print_string */
#include "sgx_trts.h"
#include <stdarg.h>
#include <stdio.h> /* vsnprintf */
#include <string.h>
#include "EnclaveManager.h"
#include "EnclaveHelper.h"
//#include "TimerQueueManager.h"
#include "ControlFlowManager.h"

//#include "sgx_tae_service.h"


/*
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
int printf(const char* fmt, ...)
{
    char buf[BUFSIZ] = { '\0' };
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
    return (int)strnlen(buf, BUFSIZ - 1) + 1;
}

/***********************************************
 * ocall functions
 ***********************************************/

/*
 * Get current time in milliseconds from the REE
 */
size_t ocallGetCurrentTime(int timeUnit){
    size_t utime= 0;
    ocall_get_current_time(&utime, timeUnit);
    return utime;
}


/***************************************************
 * ecall functions
 ***************************************************/

/*
 * Enclave Initialization
 * @params:
 *  boolean isDebug (if debugging enabled),
 *  boolean isBenchmark (if benchmarking enabled)
 */
void ecall_initialize_enclave(int isDebug, int isBenchmark){
    IS_DEBUG = isDebug;
    IS_BENCHMARK = isBenchmark;
    /* plug-in other properties */

    setupEnclave();
}

/*
 * Parse device information from json string and store in a Global Struct Network
 * @params: json string with device information
 */
int ecall_setup_device_info(char *str){
    return setupDeviceInfo(str);
}

/*
 * Return the number of devices acting as a subscriber for MQTT
 * @params: none
 */
int ecall_get_num_mqtt_subscription_topic(){
    return getNumSubscriptionTopics();
}

/*
 * Get the mqtt topic names for subscriber devices
 * @params: output struct msg to store mqtt topic name, number of subscriber devices
 */
int ecall_get_mqtt_topics(struct Message *msg, size_t count){
    return getSubscriptionTopicNames(msg, count);
}

/*
 * The main entry point in the Enclave to receive the device's trigger events
 * Tasks: Receive a trigger event from outside of enclave, decrypt the event, perform rule automation
 * @params: input struct msg containing the encrypted event
 */
void ecall_did_receive_event_mqtt(struct Message *msg){
    didReceiveEventMQTT(msg);
}

/*
 * The main entry point in the Enclave to receive the rules
 * Tasks: Receive a rule from outside of enclave, decrypt the rule, check for any conflict with existing rules, process and store the rule in db if no conflict
 * @params: input struct msg containing the encrypted rule
 */
int ecall_did_receive_rule(struct Message* msg){
    return didReceiveRule(msg);
}

/*
 * Enclave rule and device events statistics fot benchmark purposes.
 */
void ecall_print_statistics(){
    printf("Total rule conflicts = %d", TOTAL_CONFLICTED_RULES);
    printf("Total retrieved rules for device events = %d", TOTAL_RETRIEVED_RULES);
    printf("Total device commands sent = %d", TOTAL_DEVICE_COMMANDS);
    if (IS_BENCHMARK) printBenchmarkResults();
}

int ecall_get_stat_num_rule_conflicts(){
    return TOTAL_CONFLICTED_RULES;
}
int ecall_get_stat_num_rule_retrieved(){
    return TOTAL_RETRIEVED_RULES;
}
int ecall_get_stat_num_device_commands(){
    return TOTAL_DEVICE_COMMANDS;
}

