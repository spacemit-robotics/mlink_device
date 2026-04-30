/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transport/hwio/tcp.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "transport/hwio.h"
#include "transport.h"
#include "utils/log.h"
#include "utils/task.h"

/* ==================== 简化版 TCP 客户端实现 ==================== */

/* 默认服务端配置，可通过环境变量覆盖：
    *   MLINK_TCP_HOST (默认 127.0.0.1)
    *   MLINK_TCP_PORT (默认 8080)
    */
#define MLINK_TCP_DEFAULT_HOST "127.0.0.1"
#define MLINK_TCP_DEFAULT_PORT 8080

#define TCP_RECV_BUFFER_SIZE 4096

/* 每个 transport 对应一份 TCP 状态，通过 hwio->priv 绑定 */
struct tcp_state {
    int sock;
    struct thread *recv_thread;
    int running;
    hwio_callback_t recv_cb;
    void *recv_ctx;
};

static struct tcp_state *tcp_get_state(struct hwio *hwio)
{
    return hwio ? (struct tcp_state *)hwio->priv : NULL;
}

/* 建立到网关的连接 */
static int tcp_connect(struct tcp_state *state)
{
    if (!state) {
        return -1;
    }
    if (state->sock >= 0) {
        return 0;
    }

    const char *host = MLINK_TCP_DEFAULT_HOST;
    int port = MLINK_TCP_DEFAULT_PORT;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        MLINK_ERROR("TCP socket create failed: %s\n", strerror(errno));
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        MLINK_ERROR("TCP invalid server address: %s\n", host);
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        MLINK_ERROR("TCP connect to %s:%d failed: %s\n", host, port, strerror(errno));
        close(sock);
        return -1;
    }

    state->sock = sock;
    MLINK_INFO("TCP connected to %s:%d\n", host, port);
    return 0;
}

static void tcp_close(struct tcp_state *state)
{
    if (state && state->sock >= 0) {
        close(state->sock);
        state->sock = -1;
        MLINK_INFO("TCP connection closed\n");
    }
}

/* 接收线程：阻塞式 recv，一有数据就回调上层 */
static void tcp_recv_thread(void *arg)
{
    struct tcp_state *state = (struct tcp_state *)arg;
    char buffer[TCP_RECV_BUFFER_SIZE];

    if (!state) {
        return;
    }

    MLINK_INFO("TCP receive thread started\n");

    while (state->running) {
        if (state->sock < 0 || !state->recv_cb) {
            /* 尚未连接或尚未注册回调，稍等再试 */
            usleep(100 * 1000);
            continue;
        }

        ssize_t n = recv(state->sock, buffer, sizeof(buffer), 0);
        if (n > 0) {
            MLINK_DEBUG("TCP received: size=%zd\n", n);
            MLINK_DEBUG("TCP received: data=%s\n", buffer);
            state->recv_cb(buffer, (size_t)n, state->recv_ctx);
        } else if (n == 0) {
            /* 服务器关闭连接 */
            MLINK_WARNG("TCP server closed connection\n");
            tcp_close(state);
            break;
        } else {
            if (errno == EINTR) {
                continue;
            }
            MLINK_ERROR("TCP recv error: %s\n", strerror(errno));
            tcp_close(state);
            break;
        }
    }

    MLINK_INFO("TCP receive thread stopped\n");
}

static int hwio_tcp_init(struct hwio *hwio)
{
    if (!hwio) {
        return -1;
    }

    struct tcp_state *state = tcp_get_state(hwio);
    if (!state) {
        state = (struct tcp_state *)calloc(1, sizeof(struct tcp_state));
        if (!state) {
            return -1;
        }
        state->sock = -1;
        state->running = 1;
        hwio->priv = state;
    }

    if (tcp_connect(state) != 0) {
        return -1;
    }

    if (!state->recv_thread) {
        state->recv_thread = thread_create(tcp_recv_thread, state, "tcp_recv", 0, 0);
        if (!state->recv_thread) {
            MLINK_ERROR("Failed to create TCP receive thread\n");
            tcp_close(state);
            state->running = 0;
            return -1;
        }
    }

    MLINK_INFO("TCP transport initialized\n");
    return 0;
}

static int hwio_tcp_deinit(struct hwio *hwio)
{
    if (!hwio) {
        return -1;
    }

    struct tcp_state *state = tcp_get_state(hwio);
    if (!state) {
        return 0;
    }

    state->running = 0;
    if (state->recv_thread) {
        thread_stop(state->recv_thread);
        state->recv_thread = NULL;
    }

    tcp_close(state);
    free(state);
    hwio->priv = NULL;

    MLINK_INFO("TCP transport deinitialized\n");
    return 0;
}

static int hwio_tcp_write(struct hwio *hwio, const void *data, size_t size)
{
    if (!hwio || !data || size == 0) {
        return -1;
    }

    struct tcp_state *state = tcp_get_state(hwio);
    if (!state || state->sock < 0) {
        return -1;
    }

    const char *buf = (const char *)data;
    size_t left = size;
    while (left > 0) {
        ssize_t n = send(state->sock, buf, left, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            MLINK_ERROR("TCP send error: %s\n", strerror(errno));
            tcp_close(state);
            return -1;
        }
        if (n == 0) {
            MLINK_ERROR("TCP send returned 0\n");
            return -1;
        }
        buf += n;
        left -= (size_t)n;
    }

    MLINK_DEBUG("TCP write: size=%zu\n", size);
    MLINK_DEBUG("TCP write: data=%s\n", (const char *)data);
    return 0;
}

static int hwio_tcp_callback(struct hwio *hwio, hwio_callback_t callback, void *ctx)
{
    if (!hwio) {
        return -1;
    }

    struct tcp_state *state = tcp_get_state(hwio);
    if (!state) {
        state = (struct tcp_state *)calloc(1, sizeof(struct tcp_state));
        if (!state) {
            return -1;
        }
        state->sock = -1;
        state->running = 1;
        hwio->priv = state;
    }

    state->recv_cb = callback;
    state->recv_ctx = ctx;

    if (callback) {
        MLINK_INFO("TCP receive callback registered\n");
    } else {
        MLINK_INFO("TCP receive callback unregistered\n");
    }
    return 0;
}

struct hwio_ops transport_tcp_ops = {
    .init = hwio_tcp_init,
    .deinit = hwio_tcp_deinit,
    .write = hwio_tcp_write,
    .callback = hwio_tcp_callback,
};

/* TCP 传输初始化函数 - 自动注册到 transport 层 */
int transport_tcp_init(void)
{
    int ret = transport_register(TRANSPORT_TYPE_TCP, &transport_tcp_ops);
    if (ret == 0) {
        MLINK_INFO("TCP transport registered successfully\n");
    } else {
        MLINK_ERROR("Failed to register TCP transport\n");
    }
    return ret;
}
