#include "sewer.h"

struct sewer_pipe_s {
    void* u_src;
    void* u_dst;
    int status;
    int id;
    char buff[BUFF_SIZE];
};

struct sewer_s {    
    sewer_connector connector;
    sewer_writer writer;
    sewer_closer closer;
    sewer_pipe_t pipes[MAX_CONNECTIONS];
    char *addr;
    int type;    
    uint16_t port;
};

#define CHECK_ID(id, from, ret)                   \
    if ((id) < 0 || (id) > MAX_CONNECTIONS) { \
        LOG(LOG_ERROR, from " invalid id %d", id); \
        return ret;                              \
    }

enum pipe_status_e {
    P_FREE,
    P_INIT,
    P_CONNECTING,
    P_HELLO,
    P_WAIT_HELLO,
    P_REPLY_HELLO,
    P_TRANS,
};

#define TO_STR(s) #s

const char *pipe_status_e_str[] = {
    TO_STR(P_FREE),
    TO_STR(P_INIT),
    TO_STR(P_CONNECTING),
    TO_STR(P_HELLO),
    TO_STR(P_WAIT_HELLO),
    TO_STR(P_REPLY_HELLO),
    TO_STR(P_TRANS),
};

#define STATUS_STR(s) pipe_status_e_str[(s)]


void sewer_connect_remote(sewer_t *sewer, int id, void *udata);
    
sewer_pipe_t*
sewer_get_pipe(sewer_t *sewer, int id)
{
    return (sewer->pipes) + id;
}

void
reset_pipe(sewer_pipe_t * pipe)
{
    pipe->u_dst = NULL;
    pipe->u_src = NULL;
    pipe->status = P_FREE;
}

void
init_pipe(sewer_pipe_t * pipe)
{
    pipe->status = P_INIT;
}

void
destroy_pipe(sewer_t *sewer, sewer_pipe_t *pipe)
{
    if (pipe->u_src) {
        sewer->closer(sewer, pipe->u_src);
    }
    
    if (pipe->u_dst) {
        sewer->closer(sewer, pipe->u_dst);
    }
    reset_pipe(pipe);
    LOG(LOG_INFO, "destory pipe [%d]", pipe->id);
}

sewer_t*
create_sewer(int type, char *next_addr, uint32_t next_port, sewer_connector connector, sewer_writer writer, sewer_closer closer)
{
    if (type != SEWER_SERVER && type != SEWER_CLIENT) {
        LOG(LOG_ERROR, "invalid type [%d]", type);
        return NULL;
    }

    sewer_t *sewer = MALLOC(sizeof(sewer_t), "create sewer");
    sewer->closer = closer;
    sewer->writer = writer;
    sewer->connector = connector;
    sewer->type = type;
    sewer->addr = next_addr;
    sewer->port = next_port;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        reset_pipe((sewer->pipes)+i);
        sewer->pipes[i].id = i;
    }
    return sewer;
}

void
sewer_trans(sewer_t *sewer, int id, char *data, size_t len, void *udata)
{
    sewer_pipe_t *pipe = sewer_get_pipe(sewer, id);
    void *u_dst = pipe->u_dst;

    if (udata == pipe->u_src) {
        u_dst = pipe->u_dst;        
    } else if (udata == pipe->u_dst) {
        u_dst = pipe->u_src;
    } else {
        LOG(LOG_ERROR, "pipe [%d] sewer_trans error unknown udata", id);
        destroy_pipe(sewer, pipe);
        return;
    }

    if (sewer->writer(sewer, data, len, u_dst) < 0) {
        LOG(LOG_ERROR, "pipe [%d] sewer_trans write error", id);
        destroy_pipe(sewer, pipe);
    }
}

void
sewer_on_hello(sewer_t *sewer, int id, char *data, size_t len, void *udata)
{
    LOG(LOG_DEBUG, "pipe [%d] on hello", id);
    sewer_pipe_t *pipe = sewer_get_pipe(sewer, id);
    
    int type = sewer->type;

    if (type == SEWER_CLIENT) {
        LOG(LOG_DEBUG, "pipe [%d] sewer_on_hello, type [%d]", id, type);
        pipe->status = P_TRANS;
    } else {
        LOG(LOG_DEBUG, "pipe [%d] sewer_on_hello, type [%d]", id, type);
        sewer_connect_remote(sewer, id, udata);
    }
  
    LOG(LOG_DEBUG, "pipe [%d] changes status [%s]", id, STATUS_STR(pipe->status));
}

void
sewer_say_hello(sewer_t *sewer, int id, void *udata)
{
    LOG(LOG_DEBUG, "pipe [%d] will say hello", id);
    
    sewer_pipe_t *pipe = sewer_get_pipe(sewer, id);

    pipe->u_dst = udata;
    if (sewer->writer(sewer, "hello", 6, pipe->u_dst) < 0) {
        destroy_pipe(sewer, pipe);
        LOG(LOG_DEBUG, "pipe [%d] sewer_say_hello write error", id);
        return;
    }
    pipe->status = P_HELLO;
    
    LOG(LOG_DEBUG, "pipe [%d] changes status [%s]", id, STATUS_STR(pipe->status));
}

void
sewer_reply_hello(sewer_t *sewer, int id, void *udata)
{
    LOG(LOG_DEBUG, "pipe [%d] will reply hello", id);
    sewer_pipe_t *pipe = sewer_get_pipe(sewer, id);

    if (sewer->writer(sewer, "hi", 6, pipe->u_src) < 0) {
        destroy_pipe(sewer, pipe);
        LOG(LOG_DEBUG, "pipe [%d] sewer_say_hello write error", id);
        return;
    }    

    pipe->u_dst = udata;
    pipe->status = P_TRANS;
    LOG(LOG_DEBUG, "pipe [%d] changes status [%s]", id, STATUS_STR(pipe->status));    
}

void
sewer_connect_remote(sewer_t *sewer, int id, void *udata)
{
    LOG(LOG_DEBUG, "pipe [%d] will connect remote [%s]:[%d]", id, sewer->addr, sewer->port);
    
    sewer_pipe_t *pipe = sewer_get_pipe(sewer, id);
    pipe->u_src = udata;
    
    if (sewer->connector(sewer, sewer->addr, sewer->port, id, pipe->u_src) != 0) {
        destroy_pipe(sewer, pipe);
        LOG(LOG_ERROR, "pipe [%d] connect remote error", id);
        return;
    }
    
    pipe->status = P_CONNECTING;
    
    LOG(LOG_DEBUG, "pipe [%d] changes status [%s]", id, STATUS_STR(pipe->status));
}

void
sewer_wait_hello(sewer_t *sewer, int id, void *udata)
{
    LOG(LOG_DEBUG, "pipe [%d] will wait for hello", id);
    
    sewer_pipe_t *pipe = sewer_get_pipe(sewer, id);
    pipe->u_src = udata;
    pipe->status = P_WAIT_HELLO;
    
    LOG(LOG_DEBUG, "pipe [%d] changes status [%s]", id, STATUS_STR(pipe->status));    
}

void
sewer_on_read(sewer_t *sewer, int id, char *data, size_t size, void* udata)
{
    CHECK_ID(id, "sewer_on_read", ;);
    sewer_pipe_t *pipe = sewer_get_pipe(sewer, id);
    int type = sewer->type;

    switch (pipe->status) {
    case P_WAIT_HELLO:
    case P_HELLO:
        sewer_on_hello(sewer, id, data, size, udata);
        break;
    case P_TRANS:
        sewer_trans(sewer, id, data, size, udata);
        break;
    default:        
        LOG(LOG_ERROR, "pipe [%d] received on status [%s]", id, STATUS_STR(pipe->status));
        break;        
    }
    return;
}

int
find_free_slot(sewer_t *sewer)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (sewer->pipes[i].status == P_FREE) {
            return i;
        }
    }
    return SEWER_CONNECTION_FULL;
}

int
sewer_on_connect(sewer_t *sewer, int id, int ok, void* udata)
{
    LOG(LOG_DEBUG, "new connection connected id [%d] ok [%d]", id, ok);
    
    if (id == SEWER_NEW_CONNECTION) {
        id = find_free_slot(sewer);
        if (id == SEWER_CONNECTION_FULL) {
            LOG(LOG_ERROR, "connection slot is full");
            sewer->closer(sewer, udata);
            return SEWER_NEW_CONNECTION;
        }
        init_pipe(sewer->pipes + id);
    }
    
    CHECK_ID(id, "sewer_on_connect", SEWER_NEW_CONNECTION);
    sewer_pipe_t *pipe = sewer_get_pipe(sewer, id);
    int type = sewer->type;

    if (ok != 0) {
        destroy_pipe(sewer, pipe);
        return SEWER_NEW_CONNECTION;
    }
    
    switch (pipe->status) {
    case P_CONNECTING:
        LOG(LOG_DEBUG, "pipe [%d] connected to remote", id);
        if (type == SEWER_CLIENT) {
            sewer_say_hello(sewer, id, udata);
        } else {
            sewer_reply_hello(sewer, id, udata);
        }
        break;
    case P_INIT:
        LOG(LOG_DEBUG, "new pipe [%d]", id);
        if (type == SEWER_CLIENT) {
            sewer_connect_remote(sewer, id, udata);
        } else {
            sewer_wait_hello(sewer, id, udata);
        }

        break;
    default:
        LOG(LOG_ERROR, "pipe [%d] has invalid status [%s]", id, STATUS_STR(pipe->status));
        sewer->closer(sewer, udata);
        break;
    }
    return id;
}

void
sewer_on_close(sewer_t* sewer, int id, void *udata)
{
    CHECK_ID(id, "sewer_on_close", ;);
    sewer_pipe_t *pipe = sewer_get_pipe(sewer, id);
    
    if (udata == pipe->u_dst)
        pipe->u_dst = NULL;
    else if (udata == pipe->u_src)
        pipe->u_src = NULL;
    
    destroy_pipe(sewer, pipe);
}

void
destory_sewer(sewer_t *sewer)
{
}
