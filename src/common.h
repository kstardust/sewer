#ifndef _SEWER_COMMON_H
#define _SEWER_COMMON_H

#include <stdlib.h>
#include <stdio.h>

// https://stackoverflow.com/a/30106751/6579345
#define INT2VOIDP(i) (void*)(uintptr_t)(i)

#define VOIDP2INT(p) (int)(uintptr_t)(p)

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
