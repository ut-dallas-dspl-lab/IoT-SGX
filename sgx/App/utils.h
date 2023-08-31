#ifndef IOT_SGX_NEW_UTILS_H
#define IOT_SGX_NEW_UTILS_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>


int find_bool_arg(int argc, char **argv, char *arg, int def);
int find_int_arg(int argc, char **argv, char *arg, int def);
float get_seconds(clock_t clocks);

void get_log_filename(char *log_filename, bool is_rule, bool is_data);
void logInfo(char *filename, char *title, char *info);
void logMemoryDetails(char *filename);
void logCheckpointInfo(char *filename, int checkpoint_count, int num_conflict, int num_retrieved, int num_commands, long maxrss, double total_time);

uint64_t getCurrentTime(int timeUnit);



#endif //IOT_SGX_NEW_UTILS_H
