#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "utils/time.h"

#define TIMESTAMP_FORMAT "%Y-%m-%d %H:%M:%S"

char *str_contructor(char *args[]);

char *timestamp()
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char *buffer = (char *)malloc(100 * sizeof(char));
    strftime(buffer, 100, TIMESTAMP_FORMAT, t);
    return buffer;
}

char *time_string()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%ld_%d", tv.tv_sec, tv.tv_usec);
    char *time_string = strndup(buffer, strlen(buffer));
    return time_string;
}
