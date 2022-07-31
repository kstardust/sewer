#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include "sewer.h"
#include "common.h"

sewer_t *sewer;

uv_loop_t *loop;

struct write_req_s {
    uv_write_t w;
    uv_buf_t buf;
};

typedef struct write_req_s write_req_t;

void
on_close(uv_handle_t *handle)
{
    int id = (int)handle->data;
    sewer_on_close(sewer, id, (void*)handle);
    FREE(handle, "");
}

void
close_connection_stream(uv_stream_t* conn)
{
    uv_close((uv_handle_t*)conn, on_close);
}

void
on_write(uv_write_t *t, int status)
{
    write_req_t *req = (write_req_t*)t;
    FREE(req->buf.base, "free write buf");
    FREE(req, "free write req");
    if (status != 0) {
        LOG(LOG_ERROR, "uv on write error: %s", uv_strerror(status));
        close_connection_stream(req->w.handle);
    }
}

void
alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    buf->base = (char*) MALLOC(suggested_size, "alloc buffer");
    buf->len = suggested_size;
}

void
on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
{
    int id = (int)client->data;
    if (nread < 0) {
        if (nread != UV_EOF) {
            LOG(LOG_ERROR, "read error: %s\n", uv_strerror(nread));
        }

        close_connection_stream((uv_stream_t*)client);
        return;
    } else {
        printf("on read %s\n", buf->base);
        sewer_on_read(sewer, id, buf->base, nread, client);
    }
}

void
on_connect(uv_connect_t* req, int status)
{
    int id = (int)req->data;
    req->handle->data = (void*)id;
    printf("on on_connect %d\n", id);
    sewer_on_connect(sewer, id, status, req->handle);
    uv_read_start((uv_stream_t *)req->handle, alloc_buffer, on_read);
}

int
uv_connector(sewer_t *sewer, const char *addr, uint16_t port, void* udata)
{
    uv_handle_t *src = (uv_handle_t *)udata;
    uv_tcp_t* s = (uv_tcp_t*)MALLOC(sizeof(uv_tcp_t), "create tcp connection");
    uv_tcp_init(loop, s);

    uv_connect_t* connect = (uv_connect_t*)MALLOC(sizeof(uv_connect_t), "create tcp connection");

    struct sockaddr_in dest;

    uv_ip4_addr(addr, port, &dest);

    s->data = src->data;
    connect->data = src->data;
    return uv_tcp_connect(connect, s, (struct sockaddr*)&dest, on_connect);
}

void
uv_closer(sewer_t *sewer, void* udata)
{
    uv_stream_t *con = (uv_stream_t *)udata;
    close_connection_stream(con);
}

size_t
uv_writer(sewer_t *sewer, const char* data, size_t size, void* udata)
{
        printf("on write %s\n", data);    
    uv_stream_t *uvs = (uv_stream_t *)udata;
    write_req_t *req = MALLOC(sizeof(write_req_t), "write_sewer");
    req->buf = uv_buf_init(MALLOC(size, "write_sewer data"), size);
    memcpy(req->buf.base, data, size);
    return uv_write((uv_write_t*)req, uvs, &req->buf, 1, on_write);
}

void
on_new_connection(uv_stream_t *server, int status)
{
    int err = status;
        
    if (err < 0) {
        LOG(LOG_ERROR, "new connection error: %s\n", uv_strerror(err));
        return;
    }

    uv_tcp_t *client = MALLOC(sizeof(uv_tcp_t), "create new client");
    uv_tcp_init(loop, client);

    err = uv_accept(server, (uv_stream_t*)client);
    if (err) {
        LOG(LOG_ERROR, "accept error: %s\n", uv_strerror(err));
        close_connection_stream((uv_stream_t*)client);
        return;
    }

    int id = sewer_on_connect(sewer, SEWER_NEW_CONNECTION, status, (void*)client);
    if (id >= 0) {
        printf("on_new_connection  id %d\n", id);
        client->data = (void*)id;
        uv_read_start((uv_stream_t *)client, alloc_buffer, on_read);
    }
}

int main(int argc, char ** argv)
{
    config_t cfg = parse_args(argc, argv);
    loop = uv_default_loop();

    char *saddr = cfg.is_client ? "localhost" : "192.168.56.101";
    uint16_t port = cfg.is_client ? 12346 : 80;
    
    sewer = create_sewer(cfg.is_client ? SEWER_CLIENT : SEWER_SERVER, saddr, port,
                         uv_connector,
                         uv_writer,
                         uv_closer);

    if (!sewer) {
        LOG(LOG_ERROR, "init failed");
        return 1;
    }
    
    if (cfg.is_client) {
        printf("is client\n");
    } else {
    }

    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    int fd;
    int on = 1;
    uv_fileno((uv_handle_t*)&server, &fd);
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    struct sockaddr_in addr;
    
    uv_ip4_addr("0.0.0.0", cfg.is_client ? 12345 : 12346, &addr);

    uv_tcp_bind(&server, (struct sockaddr*)&addr, 0);

    int r = uv_listen((uv_stream_t*)&server, 128, on_new_connection);
    if (r) {
        LOG(LOG_ERROR, "listen error %s\n", uv_strerror(r));
        return 1;
    }

    return uv_run(loop, UV_RUN_DEFAULT);
}

