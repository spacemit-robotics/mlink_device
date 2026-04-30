/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
    * 消息流转集成测试
    */

#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mlink.h"

#define MAX_MESSAGES 100
static char *captured_messages[MAX_MESSAGES];
static int captured_count = 0;

static void capture_callback(const char *payload, void *ctx)
{
    (void)ctx;
    if (captured_count < MAX_MESSAGES && payload) {
        captured_messages[captured_count] = strdup(payload);
        captured_count++;
    }
}

static void clear_captured(void)
{
    for (int i = 0; i < captured_count; i++) {
        free(captured_messages[i]);
        captured_messages[i] = NULL;
    }
    captured_count = 0;
}

static void test_message_flow_initialize(void)
{
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "it.node.msg_flow.0");
    TEST_ASSERT(server != NULL, "Server initialization");

    mlink_server_set_send_callback(server, capture_callback, NULL);
    clear_captured();

    const char *init_msg = "{\"jsonrpc\":\"2.0\",\"method\":\"initialize\",\"id\":1}";
    mlink_server_parse_message_str(server, init_msg);

    TEST_ASSERT(captured_count > 0, "Response should be captured");

    const char *response = captured_messages[captured_count - 1];
    cJSON *json = cJSON_Parse(response);
    TEST_ASSERT(json != NULL, "Response is valid JSON");
    TEST_ASSERT(json_has_string(json, "jsonrpc", "2.0"), "Has jsonrpc");
    TEST_ASSERT(json_has_number(json, "id", 1), "Has correct id");

    cJSON_Delete(json);
    clear_captured();
}

static void test_message_flow_tools_list(void)
{
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "it.node.msg_flow.0");
    TEST_ASSERT(server != NULL, "Server initialization");

    mlink_server_set_send_callback(server, capture_callback, NULL);
    clear_captured();

    /* 注册一个工具 */
    mlink_property_list_t *props = mlink_property_list_create();
    mlink_property_list_add_int(props, "value", false, 0, false, 0, false, 0);

    struct mlink_return_value tool_cb(const mlink_property_list_t *p, void *ctx)
    {
        (void)p;
        (void)ctx;
        return mlink_return_bool(true);
    }

    mlink_tool_t *tool = mlink_tool_create(
        "test.flow_tool",
        "Flow test tool",
        props,
        tool_cb,
        NULL,
        false);
    mlink_server_add_tool(server, tool);
    mlink_property_list_destroy(props);

    /* 请求工具列表 */
    const char *list_msg = "{\"jsonrpc\":\"2.0\",\"method\":\"tools/list\",\"id\":2}";
    mlink_server_parse_message_str(server, list_msg);

    TEST_ASSERT(captured_count > 0, "Response should be captured");

    const char *response = captured_messages[captured_count - 1];
    cJSON *json = cJSON_Parse(response);
    TEST_ASSERT(json != NULL, "Response is valid JSON");

    cJSON *result = cJSON_GetObjectItem(json, "result");
    TEST_ASSERT(result != NULL, "Has result");

    cJSON *tools = cJSON_GetObjectItem(result, "tools");
    TEST_ASSERT(tools != NULL, "Has tools");
    TEST_ASSERT(cJSON_IsArray(tools), "Tools is array");

    cJSON_Delete(json);
    clear_captured();
}

static void test_message_flow_tools_call(void)
{
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "it.node.msg_flow.0");
    TEST_ASSERT(server != NULL, "Server initialization");

    mlink_server_set_send_callback(server, capture_callback, NULL);
    clear_captured();

    /* 注册工具 */
    mlink_property_list_t *props = mlink_property_list_create();
    mlink_property_list_add_int(props, "value", false, 0, false, 0, false, 0);

    struct mlink_return_value tool_cb(const mlink_property_list_t *p, void *ctx)
    {
        (void)ctx;
        int v = 0;
        if (mlink_property_list_get_int(p, "value", &v)) {
            return mlink_return_int(v * 2);
        }
        return mlink_return_string("error");
    }

    mlink_tool_t *tool = mlink_tool_create(
        "test.flow_call",
        "Flow call tool",
        props,
        tool_cb,
        NULL,
        false);
    mlink_server_add_tool(server, tool);
    mlink_property_list_destroy(props);

    /* 调用工具 */
    const char *call_msg =
        "{"
        "\"jsonrpc\":\"2.0\","
        "\"method\":\"tools/call\","
        "\"id\":3,"
        "\"params\":{"
        "\"name\":\"test.flow_call\","
            "\"arguments\":{\"value\":21}"
            "}"
            "}";
    mlink_server_parse_message_str(server, call_msg);

    TEST_ASSERT(captured_count > 0, "Response should be captured");

    const char *response = captured_messages[captured_count - 1];
    cJSON *json = cJSON_Parse(response);
    TEST_ASSERT(json != NULL, "Response is valid JSON");
    TEST_ASSERT(json_has_string(json, "jsonrpc", "2.0"), "Has jsonrpc");
    TEST_ASSERT(json_has_number(json, "id", 3), "Has correct id");

    cJSON *result = cJSON_GetObjectItem(json, "result");
    TEST_ASSERT(result != NULL, "Has result");

    cJSON_Delete(json);
    clear_captured();
}

static void test_message_flow_error_handling(void)
{
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "it.node.msg_flow.0");
    TEST_ASSERT(server != NULL, "Server initialization");

    mlink_server_set_send_callback(server, capture_callback, NULL);
    clear_captured();

    /* 发送无效的 JSON */
    mlink_server_parse_message_str(server, "invalid json");
    /* 应该不会崩溃，但可能不会发送响应 */

    /* 发送未知方法的请求 */
    const char *unknown_msg =
        "{\"jsonrpc\":\"2.0\",\"method\":\"unknown.method\",\"id\":4}";
    mlink_server_parse_message_str(server, unknown_msg);

    if (captured_count > 0) {
        const char *response = captured_messages[captured_count - 1];
        cJSON *json = cJSON_Parse(response);
        if (json) {
            cJSON *error = cJSON_GetObjectItem(json, "error");
            TEST_ASSERT(error != NULL, "Error response should have error field");
            cJSON_Delete(json);
        }
    }

    clear_captured();
}

int main(void)
{
    printf("========================================\n");
    printf("   Message Flow Integration Tests\n");
    printf("========================================\n\n");

    TEST_RUN(test_message_flow_initialize);
    TEST_RUN(test_message_flow_tools_list);
    TEST_RUN(test_message_flow_tools_call);
    TEST_RUN(test_message_flow_error_handling);

    printf("\n========================================\n");
    printf("   Test Results\n");
    printf("========================================\n");
    printf("Passed: %d\n", test_passed);
    printf("Failed: %d\n", test_failed);
    printf("Total:  %d\n", test_passed + test_failed);
    printf("========================================\n");

    if (test_failed == 0) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed!\n");
        return 1;
    }
}

