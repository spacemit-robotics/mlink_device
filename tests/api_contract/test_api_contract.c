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

static struct mlink_return_value bool_tool_cb(const mlink_property_list_t *props, void *ctx)
{
    (void)props;
    bool *called = (bool *)ctx;
    if (called) {
        *called = true;
    }
    return mlink_return_bool(true);
}

static void test_property_contract(void)
{
    mlink_property_list_t *props = mlink_property_list_create();
    TEST_ASSERT(props != NULL, "property list creation");

    TEST_ASSERT(mlink_property_list_add_bool(props, "enabled", true, true), "add bool property");
    TEST_ASSERT(mlink_property_list_add_int(props, "volume", true, 7, true, 0, true, 100), "add int property");
    TEST_ASSERT(mlink_property_list_add_int(props, "speed", false, 0, true, 0, true, 100), "add required int property");
    TEST_ASSERT(mlink_property_list_add_string(props, "mode", "auto"), "add string property");

    bool enabled = false;
    TEST_ASSERT(mlink_property_list_get_bool(props, "enabled", &enabled), "get bool default");
    TEST_ASSERT(enabled == true, "bool default value");

    int volume = 0;
    TEST_ASSERT(mlink_property_list_get_int(props, "volume", &volume), "get int default");
    TEST_ASSERT(volume == 7, "int default value");

    int speed = 0;
    TEST_ASSERT(!mlink_property_list_get_int(props, "speed", &speed), "required int has no value before parsing");

    const char *mode = mlink_property_list_get_string(props, "mode");
    TEST_ASSERT(mode != NULL && strcmp(mode, "auto") == 0, "get string default");

    TEST_ASSERT(!mlink_property_list_get_bool(props, "missing", NULL), "missing bool returns false");
    TEST_ASSERT(mlink_property_list_get_string(props, "missing") == NULL, "missing string returns NULL");
    TEST_ASSERT(!mlink_property_list_add_bool(NULL, "bad", true, true), "add bool rejects NULL list");
    TEST_ASSERT(!mlink_property_list_add_int(props, NULL, true, 1, false, 0, false, 0), "add int rejects NULL name");

    mlink_property_list_destroy(props);
}

static void test_return_value_contract(void)
{
    struct mlink_return_value rv = mlink_return_bool(true);
    TEST_ASSERT(rv.type == MLINK_RETURN_TYPE_BOOL, "bool return type");
    TEST_ASSERT(rv.data.bool_value == true, "bool return value");
    mlink_return_value_free(&rv);

    rv = mlink_return_int(42);
    TEST_ASSERT(rv.type == MLINK_RETURN_TYPE_INT, "int return type");
    TEST_ASSERT(rv.data.int_value == 42, "int return value");
    mlink_return_value_free(&rv);

    rv = mlink_return_string("hello");
    TEST_ASSERT(rv.type == MLINK_RETURN_TYPE_STRING, "string return type");
    TEST_ASSERT(rv.data.string_value != NULL, "string value allocated");
    TEST_ASSERT(strcmp(rv.data.string_value, "hello") == 0, "string return value");
    mlink_return_value_free(&rv);

    cJSON *json = cJSON_CreateObject();
    TEST_ASSERT(json != NULL, "json allocation");
    cJSON_AddStringToObject(json, "status", "ok");
    rv = mlink_return_json(json);
    TEST_ASSERT(rv.type == MLINK_RETURN_TYPE_JSON, "json return type");
    TEST_ASSERT(rv.data.json_value == json, "json return value");
    mlink_return_value_free(&rv);

    struct mlink_image_content *image = (struct mlink_image_content *)calloc(1, sizeof(*image));
    TEST_ASSERT(image != NULL, "image allocation");
    image->mime_type = test_strdup("image/png");
    image->encoded_data = test_strdup("ZmFrZQ==");
    TEST_ASSERT(image->mime_type != NULL && image->encoded_data != NULL, "image content allocation");
    rv = mlink_return_image(image);
    TEST_ASSERT(rv.type == MLINK_RETURN_TYPE_IMAGE, "image return type");
    TEST_ASSERT(rv.data.image_value == image, "image return value");
    mlink_return_value_free(&rv);
}

static void test_tool_and_server_contract(void)
{
    mlink_property_list_t *props = mlink_property_list_create();
    TEST_ASSERT(props != NULL, "property list creation");
    TEST_ASSERT(mlink_property_list_add_bool(props, "enabled", true, true), "add tool property");

    bool called = false;
    mlink_tool_t *tool = mlink_tool_create(
        "contract.bool",
        "Returns true and records callback invocation",
        props,
        bool_tool_cb,
        &called,
        false);
    TEST_ASSERT(tool != NULL, "tool creation");
    mlink_tool_destroy(tool);

    tool = mlink_tool_create(NULL, "bad", props, bool_tool_cb, NULL, false);
    TEST_ASSERT(tool == NULL, "tool rejects NULL name");

    tool = mlink_tool_create("contract.bad", NULL, props, bool_tool_cb, NULL, false);
    TEST_ASSERT(tool == NULL, "tool rejects NULL description");

    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "contract-device");
    TEST_ASSERT(server != NULL, "server initialization");

    mlink_tool_t *first = mlink_tool_create("contract.duplicate", "first", props, bool_tool_cb, NULL, false);
    mlink_tool_t *second = mlink_tool_create("contract.duplicate", "second", props, bool_tool_cb, NULL, false);
    TEST_ASSERT(first != NULL && second != NULL, "duplicate test tools created");
    TEST_ASSERT(mlink_server_add_tool(server, first), "first tool registration");
    TEST_ASSERT(!mlink_server_add_tool(server, second), "duplicate tool registration rejected");
    mlink_tool_destroy(second);

    mlink_property_list_destroy(props);
}

int main(void)
{
    printf("========================================\n");
    printf("   MLINK Device API Contract Tests\n");
    printf("========================================\n");

    TEST_RUN(test_property_contract);
    TEST_RUN(test_return_value_contract);
    TEST_RUN(test_tool_and_server_contract);

    printf("Passed: %d\n", test_passed);
    printf("Failed: %d\n", test_failed);
    return test_failed == 0 ? 0 : 1;
}
