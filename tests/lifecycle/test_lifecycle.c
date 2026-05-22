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

static char *last_message;
static int response_count;

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
    free(last_message);
    last_message = payload ? test_strdup(payload) : NULL;
    TEST_ASSERT(last_message != NULL, "capture allocation");
    response_count++;
}

static struct mlink_return_value ping_cb(const mlink_property_list_t *props, void *ctx)
{
    (void)props;
    (void)ctx;
    return mlink_return_bool(true);
}

static void assert_last_text_response(const char *expected)
{
    TEST_ASSERT(last_message != NULL, "last response exists");
    cJSON *json = cJSON_Parse(last_message);
    TEST_ASSERT(json != NULL, "last response is JSON");
    cJSON *result = cJSON_GetObjectItem(json, "result");
    TEST_ASSERT(result != NULL, "last response has result");
    cJSON *content = cJSON_GetObjectItem(result, "content");
    TEST_ASSERT(content != NULL && cJSON_IsArray(content), "last response content array");
    cJSON *entry = cJSON_GetArrayItem(content, 0);
    TEST_ASSERT(entry != NULL, "last response content entry");
    TEST_ASSERT(json_has_string(entry, "text", expected), "last response text");
    cJSON_Delete(json);
}

static void test_repeated_protocol_lifecycle(void)
{
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "lifecycle-device");
    TEST_ASSERT(server != NULL, "server initialization");
    mlink_server_set_send_callback(server, capture_send_callback, NULL);

    mlink_property_list_t *props = mlink_property_list_create();
    TEST_ASSERT(props != NULL, "property list creation");
    mlink_tool_t *tool = mlink_tool_create("lifecycle.ping", "Return true", props, ping_cb, NULL, false);
    TEST_ASSERT(tool != NULL, "tool creation");
    TEST_ASSERT(mlink_server_add_tool(server, tool), "tool registration");
    mlink_property_list_destroy(props);

    for (int i = 0; i < 50; i++) {
        char request[256];
        snprintf(request, sizeof(request),
            "{\"jsonrpc\":\"2.0\",\"method\":\"initialize\",\"id\":%d}", i * 3 + 1);
        mlink_server_parse_message_str(server, request);

        snprintf(request, sizeof(request),
            "{\"jsonrpc\":\"2.0\",\"method\":\"tools/list\",\"id\":%d}", i * 3 + 2);
        mlink_server_parse_message_str(server, request);

        snprintf(request, sizeof(request),
            "{\"jsonrpc\":\"2.0\",\"method\":\"tools/call\",\"id\":%d,"
            "\"params\":{\"name\":\"lifecycle.ping\",\"arguments\":{}}}",
            i * 3 + 3);
        mlink_server_parse_message_str(server, request);
        assert_last_text_response("true");
    }

    TEST_ASSERT(response_count == 150, "all lifecycle requests received responses");
    free(last_message);
    last_message = NULL;
}

int main(void)
{
    printf("========================================\n");
    printf("   MLINK Device Lifecycle Tests\n");
    printf("========================================\n");

    TEST_RUN(test_repeated_protocol_lifecycle);

    printf("Passed: %d\n", test_passed);
    printf("Failed: %d\n", test_failed);
    return test_failed == 0 ? 0 : 1;
}
