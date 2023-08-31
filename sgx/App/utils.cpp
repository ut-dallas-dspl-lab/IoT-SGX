#include "utils.h"
#include "sgx_urts.h"
#include "Enclave_u.h"
#include <string.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>


void del_arg(int argc, char **argv, int index)
{
    int i;
    for(i = index; i < argc-1; ++i) argv[i] = argv[i+1];
    argv[i] = 0;
}

int find_bool_arg(int argc, char **argv, char *arg, int def)
{
    int i;
    for(i = 0; i < argc-1; ++i){
        if(!argv[i]) continue;
        if(0==strcmp(argv[i], arg)){
            def = atoi(argv[i+1]);
            del_arg(argc, argv, i);
            del_arg(argc, argv, i);
            break;
        }
    }
    if (def < 0 || def > 1) return 0;
    return def;
}

int find_int_arg(int argc, char **argv, char *arg, int def)
{
    int i;
    for(i = 0; i < argc-1; ++i){
        if(!argv[i]) continue;
        if(0==strcmp(argv[i], arg)){
            def = atoi(argv[i+1]);
            del_arg(argc, argv, i);
            del_arg(argc, argv, i);
            break;
        }
    }
    return def;
}

float get_seconds(clock_t clocks)
{
    return (float)clocks/CLOCKS_PER_SEC;
}

uint64_t getCurrentTime(int timeUnit){
    uint64_t us = 0;
    switch (timeUnit) {
        case NANO: {
            us = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::
                                                                      now().time_since_epoch()).count();
            break;
        }
        case MICRO: {
            us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::
                                                                       now().time_since_epoch()).count();
            break;
        }
        case MILLI: {
            us = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::
                                                                       now().time_since_epoch()).count();
            break;
        }
        case SECOND: {
            us = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::
                                                                  now().time_since_epoch()).count();
            break;
        }
        case MINUTE: {
            us = std::chrono::duration_cast<std::chrono::minutes>(std::chrono::high_resolution_clock::
                                                                  now().time_since_epoch()).count();
            break;
        }
        default: {
            printf("ERROR! Incorrect time unit!\n");
            us = -1;
        }
    }

    return us;
}

void append_datetime(char *date_time_str){
    time_t ltime;
    struct tm result;

    ltime = time(NULL);
    localtime_r(&ltime, &result);

    char str[10];

    sprintf(str, "m%d", result.tm_mon+1);
    strcat(date_time_str, str);
    sprintf(str, "d%d", result.tm_mday);
    strcat(date_time_str, str);
    sprintf(str, "y%d", 1900+result.tm_year);
    strcat(date_time_str, str);
    sprintf(str, "_h%d", result.tm_hour);
    strcat(date_time_str, str);
    sprintf(str, "m%d", result.tm_min);
    strcat(date_time_str, str);
    sprintf(str, "s%d", result.tm_sec);
    strcat(date_time_str, str);
}

void get_log_filename(char *log_filename, bool is_rule, bool is_data){
    char output_dir[80];
    strcpy(output_dir, "logs/output_");
    is_rule? strcat(output_dir, "r1_") : strcat(output_dir, "r0_");
    is_data? strcat(output_dir, "d1_") : strcat(output_dir, "d0_");

    append_datetime(output_dir);
    strcat(output_dir, ".txt");

    printf("output file: %s\n", output_dir);
    strncpy(log_filename, output_dir, strlen(output_dir));
}

void getMemoryDetails(FILE *log_file) {
    /* For more info: https://man7.org/linux/man-pages/man5/procfs.5.html*/
    // stores each word in status file
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("/proc/self/status", "r");
    if (fp){
        //printf("/proc/self/status\n");
        fprintf(log_file, "\n\n/proc/self/status\n");
        while ((read = getline(&line, &len, fp)) != -1) {
            //printf("Retrieved line of length %zu:\n", read);
            //printf("%s", line);
            fprintf(log_file, "%s", line);
        }
    }
    fclose(fp);

//    fp = fopen("/proc/self/smaps", "r");
//    if (fp){
//        printf("/proc/self/smaps\n");
//        fprintf(output_file, "\n\n/proc/self/smaps\n");
//        while ((read = getline(&line, &len, fp)) != -1) {
//            //printf("Retrieved line of length %zu:\n", read);
//            //printf("%s", line);
//            fprintf(output_file, "%s", line);
//        }
//    }
//    fclose(fp);
}

void logInfo(char *filename, char *title, char *info){
    FILE *log_file = fopen(filename, "a+");
    fprintf(log_file, "%s: %s\n", title, info);
    fclose(log_file);
}

void logMemoryDetails(char *filename){
    FILE *log_file = fopen(filename, "a+");
    getMemoryDetails(log_file);
    fclose(log_file);
}

void logCheckpointInfo(char *filename, int checkpoint_count, int num_conflict, int num_retrieved, int num_commands, long maxrss, double total_time){
    FILE *log_file = fopen(filename, "a+");
    fprintf(log_file, "\n\n\n ######## Checkpoint:: Count=%d ########\n", checkpoint_count);
    fprintf(log_file, "TOTAL_CONFLICTED_RULES=%d\n", num_conflict);
    fprintf(log_file, "TOTAL_RETRIEVED_RULES=%d\n", num_retrieved);
    fprintf(log_file, "TOTAL_DEVICE_COMMANDS=%d\n", num_commands);
    fprintf(log_file, "maxrss = %ld\n", maxrss);
    fprintf(log_file, "Time elapsed = %lf\n\n", total_time);
    fclose(log_file);
}


