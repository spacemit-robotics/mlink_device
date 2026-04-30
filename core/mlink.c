/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mlink.h"
#include <stdlib.h>
#include <string.h>

#include "mcp/mcp_server.h"
#include "mcp/mcp_server_protocol.h"
#include "transport/transport.h"
#include "transport/hwio/tcp.h"
#include "transport/hwio/unix.h"
#include "utils/task.h"

/* Conversion helpers - mlink and mcp return_value share the same layout and can be cast safely. */
static inline struct mcp_return_value mlink_to_mcp_return_value(const struct mlink_return_value *mlink_rv)
{
    return *(const struct mcp_return_value *)mlink_rv;
}

static inline struct mlink_return_value mcp_to_mlink_return_value(const struct mcp_return_value *mcp_rv)
{
    /* Direct cast (memory layout is identical). */
    return *(const struct mlink_return_value *)mcp_rv;
}

/* Convert transport type to string. */
static const char *transport_type_to_string(enum transport_type type)
{
    switch (type) {
        case TRANSPORT_TYPE_UNIX:
            return "unix";
        case TRANSPORT_TYPE_TCP:
            return "tcp";
        case TRANSPORT_TYPE_MQTT:
            return "mqtt";
        case TRANSPORT_TYPE_HTTP:
            return "http";
        case TRANSPORT_TYPE_WS:
            return "websocket";
        case TRANSPORT_TYPE_UART:
            return "uart";
        case TRANSPORT_TYPE_SPI:
            return "spi";
        default:
            return "unknown";
    }
}

/* Server management - glue layer between mlink, mcp, and transport. */
mlink_server_t *mlink_server_init(enum transport_type type, const char *server_name)
{
    struct mcp_server *server = mcp_server_init(server_name);
    if (!server) {
        return NULL;
    }

    /* 根据传输类型初始化对应的 transport */
    struct transport *transport = NULL;

    switch (type) {
        case TRANSPORT_TYPE_UNIX:
            if (transport_unix_init() != 0) {
                return NULL;
            }
            transport = transport_create(TRANSPORT_TYPE_UNIX, "mlink_unix");
            break;
        case TRANSPORT_TYPE_TCP:
            /* 注册 TCP transport */
            if (transport_tcp_init() != 0) {
                return NULL;
            }
            /* 创建 TCP transport 实例 */
            transport = transport_create(TRANSPORT_TYPE_TCP, "mlink_tcp");
            break;

        case TRANSPORT_TYPE_MQTT:
        case TRANSPORT_TYPE_HTTP:
        case TRANSPORT_TYPE_WS:
        case TRANSPORT_TYPE_UART:
        case TRANSPORT_TYPE_SPI:
            /* TODO: 实现其他传输类型的初始化 */
            break;

        default:
            /* 未知类型，不设置 transport */
            break;
    }

    /* 设置 transport 到 server，并发送 hello 消息 */
    if (transport) {
        const char *transport_name = transport_type_to_string(type);
        if (mcp_server_set_transport(server, transport, transport_name) != 0) {
            transport_destroy(transport);
            return NULL;
        }
    }

    return (mlink_server_t *)server;
}

void mlink_server_destroy(mlink_server_t *server)
{
    if (!server) {
        return;
    }

    struct mcp_server *mcp = (struct mcp_server *)server;

    /* Destroy background scheduler if present. */
    if (mcp->worker) {
        bg_sched_destroy(mcp->worker);
        mcp->worker = NULL;
    }

    /* Destroy transport layer if present. */
    if (mcp->transport) {
        transport_destroy(mcp->transport);
        mcp->transport = NULL;
    }

    /* Clear send callback. */
    mcp->send_callback = NULL;
    mcp->send_ctx = NULL;
}

void mlink_server_run(mlink_server_t *server)
{
    (void)server;
    /* Simple blocking run loop: mcp_server uses a background scheduler
        * to process messages; this only keeps the main thread alive.
        * Periodic tasks can be attached here in the future. */
    while (1) {
        thread_sleep(1);
    }
}

void mlink_notify_message(mlink_server_t *server,
                            const char *level,
                            const char *logger,
                            const char *text)
{
    mcp_server_send_notification_message((struct mcp_server *)server,
                                            level,
                                            logger,
                                            text);
}

void mlink_server_set_send_callback(mlink_server_t *server, mlink_send_callback_t callback, void *ctx)
{
    mcp_server_set_send_callback((struct mcp_server *)server, (mcp_server_send_callback_t)callback, ctx);
}

void mlink_server_parse_message_str(mlink_server_t *server, const char *message)
{
    mcp_server_parse_message((struct mcp_server *)server, message);
}

/* Tool management - callback wrapper structure. */
struct mlink_callback_wrapper {
    mlink_tool_callback_t callback;
    void *user_ctx;
};

static struct mcp_return_value mlink_callback_wrapper(const struct mcp_property_list *properties, void *user_ctx)
{
    struct mlink_callback_wrapper *wrapper = (struct mlink_callback_wrapper *)user_ctx;
    if (!wrapper || !wrapper->callback) {
        struct mcp_return_value rv;
        rv.type = MCP_RETURN_TYPE_STRING;
        rv.data.string_value = NULL;
        return rv;
    }
    struct mlink_return_value mlink_rv =
        wrapper->callback((const mlink_property_list_t *)properties, wrapper->user_ctx);
    return mlink_to_mcp_return_value(&mlink_rv);
}

mlink_tool_t *mlink_tool_create(const char *name,
                                const char *description,
                                const mlink_property_list_t *properties,
                                mlink_tool_callback_t callback,
                                void *user_ctx,
                                bool user_only)
{
    struct mlink_callback_wrapper *wrapper =
        (struct mlink_callback_wrapper *)malloc(sizeof(struct mlink_callback_wrapper));
    if (!wrapper) {
        return NULL;
    }
    wrapper->callback = callback;
    wrapper->user_ctx = user_ctx;

    mlink_tool_t *tool = (mlink_tool_t *)mcp_tool_create(name, description,
                                            (const struct mcp_property_list *)properties,
                                            mlink_callback_wrapper,
                                            wrapper,
                                            user_only);
    if (!tool) {
        free(wrapper);
        return NULL;
    }
    return tool;
}

void mlink_tool_destroy(mlink_tool_t *tool)
{
    if (!tool) {
        return;
    }
    struct mcp_tool *mcp_tool = (struct mcp_tool *)tool;
    /* Free callback wrapper. */
    if (mcp_tool->user_ctx) {
        struct mlink_callback_wrapper *wrapper = (struct mlink_callback_wrapper *)mcp_tool->user_ctx;
        free(wrapper);
    }
    mcp_tool_destroy(mcp_tool);
}

bool mlink_server_add_tool(mlink_server_t *server, mlink_tool_t *tool)
{
    return mcp_server_add_tool((struct mcp_server *)server, (struct mcp_tool *)tool);
}

/* Property list management. */
mlink_property_list_t *mlink_property_list_create(void)
{
    return (mlink_property_list_t *)mcp_property_list_create();
}

void mlink_property_list_destroy(mlink_property_list_t *list)
{
    mcp_property_list_destroy((struct mcp_property_list *)list);
}

bool mlink_property_list_add_bool(mlink_property_list_t *list, const char *name, bool has_default, bool default_value)
{
    return mcp_property_list_add_bool((struct mcp_property_list *)list, name, has_default, default_value);
}

bool mlink_property_list_add_int(mlink_property_list_t *list,
                                    const char *name,
                                    bool has_default,
                                    int default_value,
                                    bool has_min,
                                    int min_value,
                                    bool has_max,
                                    int max_value)
{
    return mcp_property_list_add_int((struct mcp_property_list *)list, name,
                has_default, default_value, has_min, min_value, has_max, max_value);
}

bool mlink_property_list_add_string(mlink_property_list_t *list, const char *name, const char *default_value)
{
    return mcp_property_list_add_string((struct mcp_property_list *)list, name, default_value);
}

bool mlink_property_list_get_bool(const mlink_property_list_t *list, const char *name, bool *out_value)
{
    return mcp_property_list_get_bool((const struct mcp_property_list *)list, name, out_value);
}

bool mlink_property_list_get_int(const mlink_property_list_t *list, const char *name, int *out_value)
{
    return mcp_property_list_get_int((const struct mcp_property_list *)list, name, out_value);
}

const char *mlink_property_list_get_string(const mlink_property_list_t *list, const char *name)
{
    return mcp_property_list_get_string((const struct mcp_property_list *)list, name);
}

/* Return value creation. */
struct mlink_return_value mlink_return_bool(bool value)
{
    struct mcp_return_value mcp_rv = mcp_return_bool(value);
    return mcp_to_mlink_return_value(&mcp_rv);
}

struct mlink_return_value mlink_return_int(int value)
{
    struct mcp_return_value mcp_rv = mcp_return_int(value);
    return mcp_to_mlink_return_value(&mcp_rv);
}

struct mlink_return_value mlink_return_string(const char *value)
{
    struct mcp_return_value mcp_rv = mcp_return_string(value);
    return mcp_to_mlink_return_value(&mcp_rv);
}

struct mlink_return_value mlink_return_json(cJSON *json)
{
    struct mcp_return_value mcp_rv = mcp_return_json(json);
    return mcp_to_mlink_return_value(&mcp_rv);
}

struct mlink_return_value mlink_return_image(struct mlink_image_content *image)
{
    struct mcp_return_value mcp_rv = mcp_return_image((struct mcp_image_content *)image);
    return mcp_to_mlink_return_value(&mcp_rv);
}

void mlink_return_value_free(struct mlink_return_value *value)
{
    struct mcp_return_value mcp_rv = mlink_to_mcp_return_value(value);
    mcp_return_value_free(&mcp_rv);
    /* Update value contents after free. */
    *value = mcp_to_mlink_return_value(&mcp_rv);
}

void mlink_image_content_free(struct mlink_image_content *content)
{
    mcp_image_content_free((struct mcp_image_content *)content);
}


