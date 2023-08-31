#include "utils_TA.h"
#include <ctype.h>

int string_compare(char str1[], char str2[])
{
    int ctr=0;

    while(str1[ctr]==str2[ctr])
    {
        if(str1[ctr]=='\0'||str2[ctr]=='\0')
            break;
        ctr++;
    }
    if(str1[ctr]=='\0' && str2[ctr]=='\0')
        return 1;
    else
        return 0;
}

char* toLower(char* s) {
    for(char *p=s; *p; p++) *p=tolower(*p);
    return s;
}

char* toUpper(char* s) {
    for(char *p=s; *p; p++) *p=toupper(*p);
    return s;
}

double atod_ta(char *arr)
{
    double val = 0;
    int afterdot=0;
    double scale=1;
    int neg = 0;

    if (*arr == '-') {
        arr++;
        neg = 1;
    }
    while (*arr) {
        if (afterdot) {
            scale = scale/10;
            val = val + (*arr-'0')*scale;
        } else {
            if (*arr == '.')
                afterdot++;
            else
                val = val * 10.0 + (*arr - '0');
        }
        arr++;
    }
    if(neg) return -val;
    else    return  val;
}