/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_framework.h"

#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mlink.h"

#define MAX_CAPTURED_MESSAGES 64

static char *captured_messages[MAX_CAPTURED_MESSAGES];
static int captured_count = 0;

static char *test_strdup(const char *src)
{
    size_t len = strlen(src);
    char *dst = (char *)malloc(len + 1);
    if (!dst) {
        return NULL;
    }
    memcpy(dst, src, len + 1);
    return dst;
}

static void capture_send_callback(const char *payload, void *ctx)
{
    (void)ctx;
    if (payload && captured_count < MAX_CAPTURED_MESSAGES) {
        captured_messages[captured_count] = test_strdup(payload);
        TEST_ASSERT(captured_messages[captured_count] != NULL, "capture allocation");
        captured_count++;
    }
}

static void clear_captured_messages(void)
{
    for (int i = 0; i < captured_count; i++) {
        free(captured_messages[i]);
        captured_messages[i] = NULL;
    }
    captured_count = 0;
}

static cJSON *parse_last_response(void)
{
    if (captured_count <= 0) {
        printf("[FAIL] %s:%d: response captured\n", __FILE__, __LINE__);
        test_failed++;
        return NULL;
    }
    cJSON *json = cJSON_Parse(captured_messages[captured_count - 1]);
    if (!json) {
        printf("[FAIL] %s:%d: response is valid JSON\n", __FILE__, __LINE__);
        test_failed++;
        return NULL;
    }
    return json;
}

static cJSON *send_request(mlink_server_t *server, const char *request)
{
    clear_captured_messages();
    mlink_server_parse_message_str(server, request);
    return parse_last_response();
}

static cJSON *object_item(cJSON *json, const char *key)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (!item) {
        printf("[FAIL] %s:%d: JSON item exists: %s\n", __FILE__, __LINE__, key);
        test_failed++;
        return NULL;
    }
    return item;
}

static struct mlink_return_value multiply_cb(const mlink_property_list_t *props, void *ctx)
{
    (void)ctx;
    int value = 0;
    if (!mlink_property_list_get_int(props, "value", &value)) {
        return mlink_return_string("missing value parameter");
    }
    return mlink_return_int(value * 2);
}

static void register_multiply_tool(mlink_server_t *server)
{
    mlink_property_list_t *props = mlink_property_list_create();
    TEST_ASSERT(props != NULL, "property list creation");
    TEST_ASSERT(mlink_property_list_add_int(props, "value", false, 0, false, 0, false, 0), "add value property");

    mlink_tool_t *tool = mlink_tool_create(
        "protocol.multiply",
        "Multiply an integer by 2",
        props,
        multiply_cb,
        NULL,
        false);
    TEST_ASSERT(tool != NULL, "tool creation");
    TEST_ASSERT(mlink_server_add_tool(server, tool), "tool registration");
    mlink_property_list_destroy(props);
}

static void test_protocol_flow(void)
{
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "protocol-device");
    TEST_ASSERT(server != NULL, "server initialization");
    mlink_server_set_send_callback(server, capture_send_callback, NULL);
    register_multiply_tool(server);

    cJSON *json = send_request(server, "{\"jsonrpc\":\"2.0\",\"method\":\"initialize\",\"id\":1}");
    TEST_ASSERT(json_has_string(json, "jsonrpc", "2.0"), "initialize jsonrpc");
    TEST_ASSERT(json_has_number(json, "id", 1), "initialize id");
    cJSON *result = object_item(json, "result");
    cJSON *server_info = object_item(result, "serverInfo");
    TEST_ASSERT(json_has_string(server_info, "name", "protocol-device"), "initialize server name");
    cJSON_Delete(json);

    json = send_request(server, "{\"jsonrpc\":\"2.0\",\"method\":\"tools/list\",\"id\":2}");
    TEST_ASSERT(json_has_number(json, "id", 2), "tools/list id");
    result = object_item(json, "result");
    cJSON *tools = object_item(result, "tools");
    TEST_ASSERT(cJSON_IsArray(tools), "tools is array");

    bool found_tool = false;
    cJSON *tool = NULL;
    cJSON_ArrayForEach(tool, tools) {
        cJSON *name = cJSON_GetObjectItem(tool, "name");
        if (cJSON_IsString(name) && strcmp(name->valuestring, "protocol.multiply") == 0) {
            found_tool = true;
            cJSON *schema = cJSON_GetObjectItem(tool, "inputSchema");
            TEST_ASSERT(schema != NULL && cJSON_IsObject(schema), "tool has input schema");
            cJSON *properties = cJSON_GetObjectItem(schema, "properties");
            TEST_ASSERT(properties != NULL && cJSON_IsObject(properties), "tool schema has properties");
            TEST_ASSERT(cJSON_GetObjectItem(properties, "value") != NULL, "tool schema has value property");
            break;
        }
    }
    TEST_ASSERT(found_tool, "registered tool is listed");
    cJSON_Delete(json);

    json = send_request(
        server,
        "{\"jsonrpc\":\"2.0\",\"method\":\"tools/call\",\"id\":3,"
        "\"params\":{\"name\":\"protocol.multiply\",\"arguments\":{\"value\":42}}}");
    TEST_ASSERT(json_has_number(json, "id", 3), "tools/call id");
    result = object_item(json, "result");
    cJSON *content = object_item(result, "content");
    TEST_ASSERT(cJSON_IsArray(content), "call result content array");
    cJSON *entry = cJSON_GetArrayItem(content, 0);
    TEST_ASSERT(entry != NULL, "call result content entry");
    TEST_ASSERT(json_has_string(entry, "type", "text"), "call result type");
    TEST_ASSERT(json_has_string(entry, "text", "84"), "call result value");
    cJSON_Delete(json);

    json = send_request(
        server,
        "{\"jsonrpc\":\"2.0\",\"method\":\"tools/call\",\"id\":4,"
        "\"params\":{\"name\":\"protocol.missing\",\"arguments\":{}}}");
    TEST_ASSERT(cJSON_GetObjectItem(json, "error") != NULL, "unknown tool returns error");
    cJSON_Delete(json);

    int before_invalid_json = captured_count;
    mlink_server_parse_message_str(server, "not valid json");
    TEST_ASSERT(captured_count == before_invalid_json, "invalid JSON does not emit a bogus response");

    clear_captured_messages();
}

int main(void)
{
    printf("========================================\n");
    printf("   MLINK Device Protocol E2E Tests\n");
    printf("========================================\n");

    TEST_RUN(test_protocol_flow);

    printf("Passed: %d\n", test_passed);
    printf("Failed: %d\n", test_failed);
    return test_failed == 0 ? 0 : 1;
}
