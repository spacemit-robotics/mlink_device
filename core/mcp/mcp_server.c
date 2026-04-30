/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mcp/mcp_server.h"
#include <stdlib.h>
#include <string.h>
#include "mcp/mcp_tool.h"
#include "mcp/mcp_server_protocol.h"
#include "transport/transport.h"
#include "utils/log.h"
#include "utils/background_sched.h"

static struct mcp_server g_mcp_server;
static bool g_mcp_server_initialized = false;

struct mcp_server *mcp_server_init(const char *server_name)
{
    if (!g_mcp_server_initialized) {
        memset(&g_mcp_server, 0, sizeof(g_mcp_server));
        /* 创建后台调度器，用于并行处理接收的消息 */
                g_mcp_server.worker = bg_sched_create("mcp_worker", 0, 0);
                if (!g_mcp_server.worker) {
                    MLINK_WARNG("Failed to create MCP background scheduler\n");
                }
        /* 默认服务器名称（可通过参数覆盖） */
        strncpy(g_mcp_server.server_name, "MCP_SERVER", sizeof(g_mcp_server.server_name) - 1);
        g_mcp_server.server_name[sizeof(g_mcp_server.server_name) - 1] = '\0';
        /* 如果调用方提供了自定义名称，覆盖默认值 */
        if (server_name && server_name[0] != '\0') {
            strncpy(g_mcp_server.server_name, server_name, sizeof(g_mcp_server.server_name) - 1);
            g_mcp_server.server_name[sizeof(g_mcp_server.server_name) - 1] = '\0';
        }
        g_mcp_server_initialized = true;
    }
    return &g_mcp_server;
}

/* 简单的字符串哈希函数（用于工具查找优化） */
static unsigned char tool_name_hash(const char *name)
{
    if (!name || !name[0]) {
        return 0;
    }
    /* 使用名称首字符作为哈希值（简单但有效） */
    return (unsigned char)name[0];
}

/* 在哈希表中查找工具 */
struct mcp_tool *mcp_server_find_tool_by_hash(struct mcp_server *server, const char *name)
{
    if (!server || !name) {
        return NULL;
    }
    unsigned char hash = tool_name_hash(name);
    for (size_t i = 0; i < server->tool_hash_count[hash]; ++i) {
        if (server->tool_hash[hash][i] &&
            strcmp(server->tool_hash[hash][i]->name, name) == 0) {
            return server->tool_hash[hash][i];
        }
    }
    return NULL;
}

/* 将工具添加到哈希表 */
static bool mcp_server_add_tool_to_hash(struct mcp_server *server, struct mcp_tool *tool)
{
    if (!server || !tool || !tool->name) {
        return false;
    }
    unsigned char hash = tool_name_hash(tool->name);
    size_t count = server->tool_hash_count[hash];

    /* 扩展哈希桶（如果需要） */
    if (count == 0) {
        server->tool_hash[hash] = (struct mcp_tool **)malloc(sizeof(struct mcp_tool *));
        if (!server->tool_hash[hash]) {
            return false;
        }
    } else {
        struct mcp_tool **new_hash = (struct mcp_tool **)realloc(
            server->tool_hash[hash],
            (count + 1) * sizeof(struct mcp_tool *));
        if (!new_hash) {
            return false;
        }
        server->tool_hash[hash] = new_hash;
    }

    server->tool_hash[hash][count] = tool;
    server->tool_hash_count[hash] = count + 1;
    return true;
}

void mcp_server_set_send_callback(struct mcp_server *server, mcp_server_send_callback_t callback, void *ctx)
{
    if (!server) {
        return;
    }
    server->send_callback = callback;
    server->send_ctx = ctx;
}

static bool mcp_server_reserve_tools(struct mcp_server *server, size_t required)
{
    if (required <= server->capacity) {
        return true;
    }
    size_t new_capacity = server->capacity == 0 ? 4 : server->capacity * 2;
    while (new_capacity < required) {
        new_capacity *= 2;
    }
    struct mcp_tool **tools = (struct mcp_tool **)realloc(server->tools, new_capacity * sizeof(struct mcp_tool *));
    if (!tools) {
        return false;
    }
    server->tools = tools;
    server->capacity = new_capacity;
    return true;
}

bool mcp_server_add_tool(struct mcp_server *server, struct mcp_tool *tool)
{
    if (!server || !tool) {
        return false;
    }

    /* 使用哈希表快速检查重复 */
            if (mcp_server_find_tool_by_hash(server, tool->name)) {
                MLINK_WARNG("Tool %s already exists\n", tool->name);
            return false;
    }

    if (!mcp_server_reserve_tools(server, server->count + 1)) {
        return false;
    }
    server->tools[server->count++] = tool;

    /* 添加到哈希表 */
            if (!mcp_server_add_tool_to_hash(server, tool)) {
                /* 哈希表添加失败不影响主列表，但查找性能会下降 */
                MLINK_WARNG("Failed to add tool to hash table\n");
    }

    return true;
}

void mcp_server_send(struct mcp_server *server, const char *payload)
{
    if (!server) {
        return;
    }

    /* 优先使用 transport 发送 */
    if (server->transport && payload) {
        size_t len = strlen(payload);
        transport_send(server->transport, payload, (uint32_t)len);
        /* 注意：即使 transport 发送成功，也继续执行下面的 callback */
    }

    /* 同时使用回调（用于测试和日志） */
    if (server->send_callback) {
        server->send_callback(payload, server->send_ctx);
            } else if (!server->transport) {
                /* 只有在没有 transport 且没有 callback 时才打印日志 */
                MLINK_INFO("Send MCP payload: %s\n", payload);
    }
}

/* 用于后台调度的消息任务 */
struct mcp_message_task {
    struct mcp_server *server;
    char *message;
};

static void mcp_message_task_run(void *arg)
{
    struct mcp_message_task *task = (struct mcp_message_task *)arg;
    if (!task) {
        return;
    }
    if (task->server && task->message) {
        mcp_server_parse_message(task->server, task->message);
    }
    free(task->message);
    free(task);
}

/* Transport 接收数据的回调函数 */
static void mcp_server_transport_receive_callback(const void *data, size_t size, void *ctx)
{
    struct mcp_server *server = (struct mcp_server *)ctx;
    if (!server || !data || size == 0) {
        return;
    }

    /* 将接收到的数据转换为字符串 */
    char *message = (char *)malloc(size + 1);
    if (!message) {
        return;
    }
    memcpy(message, data, size);
    message[size] = '\0';

    /* 如果有后台调度器，则异步处理消息，否则同步处理 */
    if (server->worker) {
        struct mcp_message_task *task = (struct mcp_message_task *)malloc(sizeof(struct mcp_message_task));
        if (!task) {
            /* 内存不足，退回同步处理 */
            mcp_server_parse_message(server, message);
            free(message);
            return;
        }
        task->server = server;
        task->message = message;
        if (bg_sched_post(server->worker, mcp_message_task_run, task) != 0) {
            /* 调度失败，退回同步处理，避免丢消息 */
            mcp_server_parse_message(server, message);
            free(message);
            free(task);
        }
    } else {
        /* 没有worker，按原先方式同步处理 */
        mcp_server_parse_message(server, message);
        free(message);
    }
}

int mcp_server_set_transport(struct mcp_server *server, struct transport *transport, const char *transport_name)
{
    if (!server) {
        return -1;
    }

    server->transport = transport;

    /* 如果设置了 transport，注册接收回调 */
    if (transport) {
                if (transport_register_callback(transport, mcp_server_transport_receive_callback, server) != 0) {
                    MLINK_WARNG("Failed to register transport receive callback\n");
            return -1;
        }

        /* 发送 hello 消息建立连接和能力通告 */
        if (transport_name) {
            mcp_server_send_hello(server, transport_name);
        }
    }

    return 0;
}

