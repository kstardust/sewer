#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define COMMON_C
#include "common.h"

config_t
parse_args(int argc, char **argv)
{
    char c;
    config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    while ((c = getopt(argc, argv, "cf:")) != -1) {
        switch (c) {
        case 'c':
            cfg.is_client = 1;
        case 'f':
            LOG(LOG_DEBUG, "f optarg = %s\n", optarg);
            break;
        default:
            LOG(LOG_ERROR, "invalid arguments\n");
            exit(1);
        }
    }
    return cfg;
}


