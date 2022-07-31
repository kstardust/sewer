#ifndef _SEWER_COMMON_H
#define _SEWER_COMMON_H

#include <stdlib.h>
#include <stdio.h>

#ifndef COMMON_C
extern char *LOG_STRING[3];
#else
char *LOG_STRING[3] = { "DEBUG", "INFO", "ERROR" };
#endif

#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_ERROR 2

#define MALLOC(x, r) malloc((x))

#define FREE(x, r) \
    x ? free((x)), x = NULL : x;

#define LOG(level, ...) \
    printf("[%s]: ", LOG_STRING[(level)]); \
    printf(__VA_ARGS__); \
    putchar('\n')

typedef struct config_s config_t;

struct config_s {
    int is_client;
};

config_t parse_args(int argc, char **argv);


    
#endif
