#ifndef OPTEE_EXAMPLE_IOT_UTILS_H
#define OPTEE_EXAMPLE_IOT_UTILS_H

int find_bool_arg(int argc, char **argv, char *arg, int def);
int find_int_arg(int argc, char **argv, char *arg, int def);
char *string_trim_inplace(char *s);
#endif //OPTEE_EXAMPLE_IOT_UTILS_H
