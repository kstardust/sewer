#ifndef _SEWER_H
#define _SEWER_H

#include <uv.h>
#include "common.h"

#define MAX_CONNECTIONS 4096
#define BUFF_SIZE 4096


#define SEWER_NEW_CONNECTION -1
#define SEWER_CONNECTION_FULL -2

#define SEWER_CLIENT 0
#define SEWER_SERVER 1

struct sewer_s;
struct sewer_pipe_s;

typedef struct sewer_s sewer_t;
typedef struct sewer_pipe_s sewer_pipe_t;

typedef int (*sewer_connector)(sewer_t *sewer, const char *addr, uint16_t port, int starter_id, void* udata);
typedef size_t (*sewer_writer)(sewer_t *sewer, const char* data, size_t len, void* udata);
typedef void (*sewer_closer)(sewer_t *sewer, void* udata);

sewer_t* create_sewer(int type, char *next_addr, uint32_t next_port, sewer_connector connector, sewer_writer writer, sewer_closer closer);
void destory_sewer(sewer_t *sewer);
void sewer_on_read(sewer_t *sewer, int id, char *data, size_t size, void* udata);
int sewer_on_connect(sewer_t *sewer, int id, int ok, void* udata);
void sewer_on_close(sewer_t* sewer, int id, void *udata);

    
#endif
