/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mcp/mcp_server_protocol.h"
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cJSON.h>
#include "mcp/mcp_server.h"
#include "mcp/mcp_tool.h"
#include "mcp/mcp_property.h"
#include "utils/list.h"
#include "utils/log.h"

/* 前向声明（函数在mcp_server.c中实现） */
struct mcp_tool *mcp_server_find_tool_by_hash(struct mcp_server *server, const char *name);

#define MCP_SERVER_MAX_PAYLOAD 8000

static void mcp_server_reply_result(struct mcp_server *server, int id, const char *result)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(root, "id", id);
    cJSON *result_json = cJSON_Parse(result);
    if (!result_json) {
        result_json = cJSON_CreateRaw(result);
    }
    cJSON_AddItemToObject(root, "result", result_json);
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    mcp_server_send(server, payload);
    free(payload);
}

static void mcp_server_reply_error(struct mcp_server *server, int id, const char *message)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(root, "id", id);

    cJSON *error = cJSON_CreateObject();
    cJSON_AddStringToObject(error, "message", message);
    cJSON_AddItemToObject(root, "error", error);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    mcp_server_send(server, payload);
    free(payload);
}

static void mcp_server_add_server_info(struct mcp_server *server, cJSON *root)
{
    cJSON *server_info = cJSON_CreateObject();
    const char *name = (server && server->server_name[0]) ? server->server_name : "MCP_SERVER";
    cJSON_AddStringToObject(server_info, "name", name);
    cJSON_AddStringToObject(server_info, "version", "1.0.0");
    cJSON_AddItemToObject(root, "serverInfo", server_info);
}

static char *mcp_server_build_initialize_result(struct mcp_server *server)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "protocolVersion", MCP_SERVER_PROTOCOL_VERSION);

    cJSON *capabilities = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "capabilities", capabilities);
    cJSON_AddItemToObject(capabilities, "tools", cJSON_CreateObject());

    mcp_server_add_server_info(server, root);

    char *result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return result;
}

static char *mcp_server_build_tools_list(struct mcp_server *server, const char *cursor, bool include_user_tools)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *tools = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "tools", tools);

    bool start = !cursor || cursor[0] == '\0';
    char next_cursor[64] = {0};
    size_t estimated_size = 50; /* 基础JSON结构大小估计 */

    for (size_t i = 0; i < server->count; ++i) {
        struct mcp_tool *tool = server->tools[i];
        if (!include_user_tools && tool->user_only) {
            continue;
        }
        if (!start) {
            if (strcmp(tool->name, cursor) == 0) {
                start = true;
            } else {
                continue;
            }
        }

        /* 估算工具JSON大小（名称+描述+属性，粗略估计） */
        size_t tool_size_estimate = strlen(tool->name) + strlen(tool->description) + 200;
        if (estimated_size + tool_size_estimate > MCP_SERVER_MAX_PAYLOAD) {
            strncpy(next_cursor, tool->name, sizeof(next_cursor) - 1);
            break;
        }

        char *tool_json = mcp_tool_to_json(tool);
        if (!tool_json) {
            continue;
        }

        /* 实际检查大小（只在接近限制时） */
        if (estimated_size + strlen(tool_json) > MCP_SERVER_MAX_PAYLOAD * 0.9) {
            char *root_json = cJSON_PrintUnformatted(root);
            size_t root_json_len = root_json ? strlen(root_json) : 0;
            if (strlen(tool_json) + root_json_len > MCP_SERVER_MAX_PAYLOAD) {
                if (root_json) free(root_json);
            strncpy(next_cursor, tool->name, sizeof(next_cursor) - 1);
            free(tool_json);
            break;
        }
            if (root_json) free(root_json);
        }

        cJSON *tool_object = cJSON_Parse(tool_json);
        free(tool_json);
        if (!tool_object) {
            continue;
        }
        cJSON_AddItemToArray(tools, tool_object);
        estimated_size += tool_size_estimate;
    }

    if (next_cursor[0] != '\0') {
        cJSON_AddStringToObject(root, "nextCursor", next_cursor);
    }

    char *result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return result;
}

static struct mcp_property_list mcp_server_parse_arguments(
    const struct mcp_tool *tool, const cJSON *arguments, bool *ok, const char **error_property)
{
    struct mcp_property_list parsed = mcp_property_list_clone(&tool->properties);
    *ok = true;
    struct list *l = PROPERTY_LIST_TO_LIST(&parsed);
    size_t count = list_count(l);
    for (size_t i = 0; i < count; ++i) {
        struct mcp_property *property = (struct mcp_property *)list_get(l, i);
        const cJSON *value = arguments ? cJSON_GetObjectItem(arguments, property->name) : NULL;
        if (!value) {
            if (!property->has_default) {
                *ok = false;
                *error_property = property->name;
            }
            continue;
        }
        if (!mcp_property_copy_from_json(property, value)) {
            *ok = false;
            *error_property = property->name;
            break;
        }
    }
    return parsed;
}

static void mcp_server_do_tool_call(struct mcp_server *server, int id, const char *tool_name, const cJSON *arguments)
{
    /* 使用哈希表快速查找工具（O(1)平均情况） */
    struct mcp_tool *tool = mcp_server_find_tool_by_hash(server, tool_name);
    if (!tool) {
        /* 工具未找到 */
        char message[128];
        snprintf(message, sizeof(message), "Unknown tool: %s", tool_name);
        mcp_server_reply_error(server, id, message);
        return;
        }

        bool ok = false;
        const char *error_property = NULL;
        struct mcp_property_list parsed = mcp_server_parse_arguments(tool, arguments, &ok, &error_property);
        if (!ok) {
            char message[128];
            snprintf(message, sizeof(message), "Invalid argument: %s", error_property ? error_property : "unknown");
            mcp_server_reply_error(server, id, message);
            mcp_property_list_free(&parsed);
            return;
        }
        char *result = mcp_tool_call(tool, &parsed);
        if (result) {
            mcp_server_reply_result(server, id, result);
            free(result);
        } else {
            mcp_server_reply_error(server, id, "Tool call failed");
        }
        mcp_property_list_free(&parsed);
}

static void mcp_server_parse_message_json(struct mcp_server *server, const cJSON *json)
{
            if (!server || !json) {
                return;
            }

            const cJSON *version = cJSON_GetObjectItem(json, "jsonrpc");
            if (!cJSON_IsString(version) || strcmp(version->valuestring, "2.0") != 0) {
                MLINK_ERROR("Invalid JSONRPC version\n");
                return;
            }

            const cJSON *method = cJSON_GetObjectItem(json, "method");
            if (!cJSON_IsString(method)) {
                MLINK_ERROR("Missing method\n");
                return;
            }

            const cJSON *params = cJSON_GetObjectItem(json, "params");
            const cJSON *id = cJSON_GetObjectItem(json, "id");
            if (!cJSON_IsNumber(id)) {
                MLINK_ERROR("Missing id\n");
                return;
            }
    int id_value = id->valueint;

    if (strcmp(method->valuestring, "initialize") == 0) {
        char *initialize_result = mcp_server_build_initialize_result(server);
        mcp_server_reply_result(server, id_value, initialize_result);
        free(initialize_result);
        return;
    }

    if (strcmp(method->valuestring, "tools/list") == 0) {
        const char *cursor = NULL;
        bool include_user_tools = false;
        if (params) {
            const cJSON *cursor_value = cJSON_GetObjectItem(params, "cursor");
            if (cJSON_IsString(cursor_value)) {
                cursor = cursor_value->valuestring;
            }
            const cJSON *with_user_tools = cJSON_GetObjectItem(params, "withUserTools");
            if (cJSON_IsBool(with_user_tools)) {
                include_user_tools = with_user_tools->valueint == 1;
            }
        }
        char *tools_result = mcp_server_build_tools_list(server, cursor, include_user_tools);
        if (tools_result) {
            mcp_server_reply_result(server, id_value, tools_result);
            free(tools_result);
        } else {
            mcp_server_reply_error(server, id_value, "Failed to generate tools list");
        }
        return;
    }

    if (strcmp(method->valuestring, "tools/call") == 0) {
        if (!params) {
            mcp_server_reply_error(server, id_value, "Missing params");
            return;
        }
        const cJSON *name = cJSON_GetObjectItem(params, "name");
        if (!cJSON_IsString(name)) {
            mcp_server_reply_error(server, id_value, "Missing tool name");
            return;
        }
        const cJSON *arguments = cJSON_GetObjectItem(params, "arguments");
        if (arguments && !cJSON_IsObject(arguments)) {
            mcp_server_reply_error(server, id_value, "Invalid arguments");
            return;
        }
        mcp_server_do_tool_call(server, id_value, name->valuestring, arguments);
        return;
    }

    char message[128];
    snprintf(message, sizeof(message), "Method not implemented: %s", method->valuestring);
    mcp_server_reply_error(server, id_value, message);
}

/* 生成会话ID */
static char *mcp_generate_session_id(void)
{
    static unsigned int counter = 0;
    const time_t now = time(NULL);
    char session_id_buf[64];
    int len = snprintf(
        session_id_buf, sizeof(session_id_buf), "mcp_%" PRIi64 "_%u", (int64_t)now, counter++);
    if (len < 0) {
        return NULL;
    }
    char *session_id = (char *)malloc((size_t)len + 1U);
    if (!session_id) {
        return NULL;
    }
    memcpy(session_id, session_id_buf, (size_t)len + 1U);
    return session_id;
}

/* 发送 hello 消息建立连接和能力通告 */
void mcp_server_send_hello(struct mcp_server *server, const char *transport_name)
{
    if (!server || !transport_name) {
        return;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return;
    }

    /* 设置消息类型 */
    cJSON_AddStringToObject(root, "type", "hello");

    /* 设置协议版本 */
    cJSON_AddStringToObject(root, "version", MCP_SERVER_PROTOCOL_VERSION);

    /* 设置特性 */
    cJSON *features = cJSON_CreateObject();
    if (features) {
        cJSON_AddBoolToObject(features, "mcp", true);
        cJSON_AddItemToObject(root, "features", features);
    }

    /* 设置传输协议类型 */
    cJSON_AddStringToObject(root, "transport", transport_name);

    /* 生成并设置会话ID */
    char *session_id = mcp_generate_session_id();
    if (session_id) {
        cJSON_AddStringToObject(root, "session_id", session_id);
        free(session_id);
    }

    /* 转换为 JSON 字符串并发送 */
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        mcp_server_send(server, json_str);
        free(json_str);
    }

    cJSON_Delete(root);
}

void mcp_server_parse_message(struct mcp_server *server, const char *message)
{
    cJSON *json = cJSON_Parse(message);
            if (!json) {
                MLINK_ERROR("Failed to parse MCP message: %s\n", message);
                return;
            }
    mcp_server_parse_message_json(server, json);
    cJSON_Delete(json);
}

/* 发送 notifications/message 日志通知 */
void mcp_server_send_notification_message(struct mcp_server *server,
                                            const char *level,
                                            const char *logger,
                                            const char *text)
{
    if (!server) {
        return;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return;
    }

    cJSON_AddStringToObject(root, "jsonrpc", "2.0");
    cJSON_AddStringToObject(root, "method", "notifications/message");

    cJSON *params = cJSON_CreateObject();
    if (params) {
        cJSON_AddStringToObject(params, "level", level ? level : "info");
        if (logger) {
            cJSON_AddStringToObject(params, "logger", logger);
        }
        if (text) {
            cJSON_AddStringToObject(params, "data", text);
        }
        cJSON_AddItemToObject(root, "params", params);
    }

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!payload) {
        return;
    }

    mcp_server_send(server, payload);
    free(payload);
}

