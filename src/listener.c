#include "growtopia/listener.h"
#include "growtopia/security.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

static h2o_context_t *g_ctx = NULL;
static h2o_accept_ctx_t *g_accept_ctx = NULL;

int listener_init(h2o_context_t *ctx, h2o_accept_ctx_t *accept_ctx)
{
    g_ctx = ctx;
    g_accept_ctx = accept_ctx;
    return 0;
}

#ifdef H2O_USE_LIBUV
static uv_tcp_t listener;

static void on_accept_uv(uv_stream_t *listener_stream, int status)
{
    uv_tcp_t *conn;
    h2o_socket_t *sock;
    struct sockaddr_storage addr;
    int addr_len = sizeof(addr);

    if (status != 0)
        return;

    conn = h2o_mem_alloc(sizeof(*conn));
    uv_tcp_init(listener_stream->loop, conn);

    if (uv_accept(listener_stream, (uv_stream_t *)conn) != 0) {
        uv_close((uv_handle_t *)conn, (uv_close_cb)free);
        return;
    }

    /* Get peer address for security checks */
    if (uv_tcp_getpeername(conn, (struct sockaddr *)&addr, &addr_len) == 0) {
        /* Check if connection is allowed */
        if (!security_check_connection((struct sockaddr *)&addr)) {
            /* Connection blocked by security */
            uv_close((uv_handle_t *)conn, (uv_close_cb)free);
            return;
        }
        /* Register the connection */
        security_register_connection((struct sockaddr *)&addr);
    }

    sock = h2o_uv_socket_create((uv_handle_t *)conn, (uv_close_cb)free);
    h2o_accept(g_accept_ctx, sock);
}

static int create_listener_uv(void)
{
    struct sockaddr_in addr;
    int r;

    uv_tcp_init(g_ctx->loop, &listener);
    uv_ip4_addr("0.0.0.0", 8000, &addr);
    if ((r = uv_tcp_bind(&listener, (const struct sockaddr *)&addr, 0)) != 0) {
        fprintf(stderr, "uv_tcp_bind:%s\n", uv_strerror(r));
        goto Error;
    }
    if ((r = uv_listen((uv_stream_t *)&listener, 128, on_accept_uv)) != 0) {
        fprintf(stderr, "uv_listen:%s\n", uv_strerror(r));
        goto Error;
    }

    return 0;
Error:
    uv_close((uv_handle_t *)&listener, NULL);
    return r;
}

int listener_start(void)
{
    return create_listener_uv();
}

void listener_stop(void)
{
    uv_close((uv_handle_t *)&listener, NULL);
}

#else /* libuv not used */

static void on_accept_ev(h2o_socket_t *listener, const char *err)
{
    h2o_socket_t *sock;

    (void)err;

    if ((sock = h2o_evloop_socket_accept(listener)) == NULL)
        return;
    h2o_accept(g_accept_ctx, sock);
}

static int create_listener_ev(void)
{
    struct sockaddr_in addr;
    int fd, reuseaddr_flag = 1;
    h2o_socket_t *sock;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(0x7f000001);
    addr.sin_port = htons(7890);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ||
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_flag, sizeof(reuseaddr_flag)) != 0 ||
        bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0 || listen(fd, SOMAXCONN) != 0) {
        return -1;
    }

    sock = h2o_evloop_socket_create(g_ctx->loop, fd, H2O_SOCKET_FLAG_DONT_READ);
    h2o_socket_read_start(sock, on_accept_ev);

    return 0;
}

int listener_start(void)
{
    return create_listener_ev();
}

void listener_stop(void)
{
    /* no-op for the simple implementation; cleanup happens on shutdown */
}

#endif
