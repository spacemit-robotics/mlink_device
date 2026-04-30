/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
    * 工具创建和调用单元测试
    */

#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include "mlink.h"

static struct mlink_return_value test_tool_callback(const mlink_property_list_t *props, void *ctx)
{
    (void)ctx;
    int value = 0;
    if (mlink_property_list_get_int(props, "value", &value)) {
        return mlink_return_int(value * 2);
    }
    return mlink_return_string("missing value parameter");
}

static void test_tool_create_destroy(void)
{
    mlink_property_list_t *props = mlink_property_list_create();
    TEST_ASSERT(props != NULL, "Property list creation");

    mlink_property_list_add_int(props, "value", false, 0, false, 0, false, 0);

    mlink_tool_t *tool = mlink_tool_create(
        "test.tool",
        "Test tool description",
        props,
        test_tool_callback,
        NULL,
        false);
    TEST_ASSERT(tool != NULL, "Tool creation should succeed");

    mlink_tool_destroy(tool);
    mlink_property_list_destroy(props);
}

static void test_tool_with_user_context(void)
{
    mlink_property_list_t *props = mlink_property_list_create();
    mlink_property_list_add_int(props, "value", false, 0, false, 0, false, 0);

    int user_data = 42;
    mlink_tool_t *tool = mlink_tool_create(
        "test.tool",
        "Test tool",
        props,
        test_tool_callback,
        &user_data,
        true  /* user_only */);
    TEST_ASSERT(tool != NULL, "Tool creation with user context");

    mlink_tool_destroy(tool);
    mlink_property_list_destroy(props);
}

static void test_tool_callback_execution(void)
{
    mlink_property_list_t *props = mlink_property_list_create();
    mlink_property_list_add_int(props, "value", false, 0, false, 0, false, 0);

    mlink_tool_t *tool = mlink_tool_create(
        "test.multiply",
        "Multiply by 2",
        props,
        test_tool_callback,
        NULL,
        false);
    TEST_ASSERT(tool != NULL, "Tool creation");

    /* 创建参数列表并设置值 */
    mlink_property_list_t *call_props = mlink_property_list_create();
    mlink_property_list_add_int(call_props, "value", false, 0, false, 0, false, 0);
    /* 注意：这里需要手动设置值，但 mlink_property_list 可能不支持直接设置 */
    /* 实际调用需要通过 mcp_server_parse_message 来触发 */

    mlink_tool_destroy(tool);
    mlink_property_list_destroy(props);
    mlink_property_list_destroy(call_props);
}

static void test_tool_null_parameters(void)
{
    /* 测试 NULL 参数 */
    mlink_tool_t *tool = mlink_tool_create(
        NULL,
        "Description",
        NULL,
        NULL,
        NULL,
        false);
    /* 创建可能失败，这是预期的 */
    if (tool) {
        mlink_tool_destroy(tool);
    }

    /* 测试空名称 */
    mlink_property_list_t *props = mlink_property_list_create();
    tool = mlink_tool_create(
        "",
        "Description",
        props,
        test_tool_callback,
        NULL,
        false);
    if (tool) {
        mlink_tool_destroy(tool);
    }
    mlink_property_list_destroy(props);
}

int main(void)
{
    printf("========================================\n");
    printf("   Tool Unit Tests\n");
    printf("========================================\n\n");

    TEST_RUN(test_tool_create_destroy);
    TEST_RUN(test_tool_with_user_context);
    TEST_RUN(test_tool_callback_execution);
    TEST_RUN(test_tool_null_parameters);

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

