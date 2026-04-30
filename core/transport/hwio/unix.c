/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transport/hwio/unix.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "transport/hwio.h"
#include "transport.h"
#include "utils/log.h"
#include "utils/task.h"

/* ==================== Unix Domain Socket 客户端实现 ==================== */

/* 默认 Unix Socket 路径 */
#define MLINK_UNIX_DEFAULT_PATH "/tmp/mlink.sock"

#define UNIX_RECV_BUFFER_SIZE 4096

struct unix_state {
    int sock;
    struct thread *recv_thread;
    int running;
    hwio_callback_t recv_cb;
    void *recv_ctx;
};

static struct unix_state *unix_get_state(struct hwio *hwio)
{
    return hwio ? (struct unix_state *)hwio->priv : NULL;
}

static int unix_connect(struct unix_state *state)
{
    if (!state) {
        return -1;
    }
    if (state->sock >= 0) {
        return 0;
    }

    const char *path = MLINK_UNIX_DEFAULT_PATH;

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        MLINK_ERROR("UNIX socket create failed: %s\n", strerror(errno));
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        MLINK_ERROR("UNIX connect to %s failed: %s\n", path, strerror(errno));
        close(sock);
        return -1;
    }

    state->sock = sock;
    MLINK_INFO("UNIX connected to %s\n", path);
    return 0;
}

static void unix_close(struct unix_state *state)
{
    if (state && state->sock >= 0) {
        close(state->sock);
        state->sock = -1;
        MLINK_INFO("UNIX connection closed\n");
    }
}

static void unix_recv_thread(void *arg)
{
    struct unix_state *state = (struct unix_state *)arg;
    char buffer[UNIX_RECV_BUFFER_SIZE];

    if (!state) {
        return;
    }

    MLINK_INFO("UNIX receive thread started\n");

    while (state->running) {
        if (state->sock < 0 || !state->recv_cb) {
            usleep(100 * 1000);
            continue;
        }

        ssize_t n = recv(state->sock, buffer, sizeof(buffer), 0);
        if (n > 0) {
            MLINK_DEBUG("UNIX received: size=%zd\n", n);
            MLINK_DEBUG("UNIX received: data=%s\n", buffer);
            state->recv_cb(buffer, (size_t)n, state->recv_ctx);
        } else if (n == 0) {
            MLINK_WARNG("UNIX server closed connection\n");
            unix_close(state);
            break;
        } else {
            if (errno == EINTR) {
                continue;
            }
            MLINK_ERROR("UNIX recv error: %s\n", strerror(errno));
            unix_close(state);
            break;
        }
    }

    MLINK_INFO("UNIX receive thread stopped\n");
}

static int hwio_unix_init(struct hwio *hwio)
{
    if (!hwio) {
        return -1;
    }

    struct unix_state *state = unix_get_state(hwio);
    if (!state) {
        state = (struct unix_state *)calloc(1, sizeof(struct unix_state));
        if (!state) {
            return -1;
        }
        state->sock = -1;
        state->running = 1;
        hwio->priv = state;
    }

    if (unix_connect(state) != 0) {
        return -1;
    }

    if (!state->recv_thread) {
        state->recv_thread = thread_create(unix_recv_thread, state, "unix_recv", 0, 0);
        if (!state->recv_thread) {
            MLINK_ERROR("Failed to create UNIX receive thread\n");
            unix_close(state);
            state->running = 0;
            return -1;
        }
    }

    MLINK_INFO("UNIX transport initialized\n");
    return 0;
}

static int hwio_unix_deinit(struct hwio *hwio)
{
    if (!hwio) {
        return -1;
    }

    struct unix_state *state = unix_get_state(hwio);
    if (!state) {
        return 0;
    }

    state->running = 0;
    if (state->recv_thread) {
        thread_stop(state->recv_thread);
        state->recv_thread = NULL;
    }

    unix_close(state);
    free(state);
    hwio->priv = NULL;

    MLINK_INFO("UNIX transport deinitialized\n");
    return 0;
}

static int hwio_unix_write(struct hwio *hwio, const void *data, size_t size)
{
    if (!hwio || !data || size == 0) {
        return -1;
    }

    struct unix_state *state = unix_get_state(hwio);
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
            MLINK_ERROR("UNIX send error: %s\n", strerror(errno));
            unix_close(state);
            return -1;
        }
        if (n == 0) {
            MLINK_ERROR("UNIX send returned 0\n");
            return -1;
        }
        buf += n;
        left -= (size_t)n;
    }

    MLINK_DEBUG("UNIX write: size=%zu\n", size);
    MLINK_DEBUG("UNIX write: data=%s\n", (const char *)data);
    return 0;
}

static int hwio_unix_callback(struct hwio *hwio, hwio_callback_t callback, void *ctx)
{
    if (!hwio) {
        return -1;
    }

    struct unix_state *state = unix_get_state(hwio);
    if (!state) {
        state = (struct unix_state *)calloc(1, sizeof(struct unix_state));
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
        MLINK_INFO("UNIX receive callback registered\n");
    } else {
        MLINK_INFO("UNIX receive callback unregistered\n");
    }

    return 0;
}

static struct hwio_ops transport_unix_ops = {
    .init = hwio_unix_init,
    .deinit = hwio_unix_deinit,
    .write = hwio_unix_write,
    .callback = hwio_unix_callback,
};

int transport_unix_init(void)
{
    int ret = transport_register(TRANSPORT_TYPE_UNIX, &transport_unix_ops);
    if (ret == 0) {
        MLINK_INFO("UNIX transport registered successfully\n");
    } else {
        MLINK_ERROR("Failed to register UNIX transport\n");
    }
    return ret;
}


