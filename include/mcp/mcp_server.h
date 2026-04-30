/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MCP_SERVER_H
#define MCP_SERVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "utils/list.h"
#include "utils/background_sched.h"
#include <cJSON.h>

struct transport;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MCP_SERVER_PROTOCOL_VERSION
#define MCP_SERVER_PROTOCOL_VERSION "2024-11-05"
#endif

struct mcp_image_content {
    char *mime_type;
    char *encoded_data;
};

enum mcp_return_type {
    MCP_RETURN_TYPE_BOOL,
    MCP_RETURN_TYPE_INT,
    MCP_RETURN_TYPE_STRING,
    MCP_RETURN_TYPE_JSON,
    MCP_RETURN_TYPE_IMAGE
};

struct mcp_return_value {
    enum mcp_return_type type;
    union {
        bool bool_value;
        int int_value;
        char *string_value;
        cJSON *json_value;
        struct mcp_image_content *image_value;
    } data;
};

enum mcp_property_type {
    MCP_PROPERTY_BOOLEAN,
    MCP_PROPERTY_INTEGER,
    MCP_PROPERTY_STRING
};

struct mcp_property {
    char *name;
    enum mcp_property_type type;
    bool has_default;
    bool has_min;
    bool has_max;
    int min_value;
    int max_value;
    union {
        bool bool_value;
        int int_value;
        char *string_value;
    } value;
};

/* mcp_property_list uses the generic list structure as its base. */
struct mcp_property_list {
    struct list base;      /* Generic list structure; must be the first member. */
};

struct mcp_tool;

typedef struct mcp_return_value (*mcp_tool_callback_t)(const struct mcp_property_list *properties, void *user_ctx);

struct mcp_tool {
    char *name;
    char *description;
    struct mcp_property_list properties;
    mcp_tool_callback_t callback;
    void *user_ctx;
    bool user_only;
};

typedef void (*mcp_server_send_callback_t)(const char *payload, void *user_ctx);

struct mcp_server {
    struct mcp_tool **tools;
    size_t count;
    size_t capacity;
    char server_name[128]; /* Instance name used for initialize.serverInfo.name. */
    mcp_server_send_callback_t send_callback;
    void *send_ctx;
    struct transport *transport;
    /* Tool name hash table (for fast lookup, 256 buckets). */
    struct mcp_tool **tool_hash[256];
    size_t tool_hash_count[256];
    /* Background scheduler used to process received messages in parallel. */
    struct sched *worker;
};

/* ==================== Included sub-module headers ==================== */
#include "mcp_return_value.h"
#include "mcp_property.h"
#include "mcp_tool.h"
#include "mcp_server_protocol.h"

/* Server management functions. */
struct mcp_server *mcp_server_init(const char *server_name);
void mcp_server_set_send_callback(struct mcp_server *server, mcp_server_send_callback_t callback, void *ctx);
bool mcp_server_add_tool(struct mcp_server *server, struct mcp_tool *tool);
void mcp_server_send(struct mcp_server *server, const char *payload);
int mcp_server_set_transport(struct mcp_server *server, struct transport *transport, const char *transport_name);
struct mcp_tool *mcp_server_find_tool_by_hash(struct mcp_server *server, const char *name);

/* Protocol-related functions are declared in mcp_server_protocol.h. */

#ifdef __cplusplus
}
#endif

#endif /* MCP_SERVER_H */

