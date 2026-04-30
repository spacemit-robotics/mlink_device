/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
    * 工具注册流程集成测试
    */

#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include "mlink.h"

static struct mlink_return_value dummy_callback(const mlink_property_list_t *props, void *ctx)
{
    (void)props;
    (void)ctx;
    return mlink_return_bool(true);
}

static void test_register_single_tool(void)
{
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "it.node.tool_reg.0");
    TEST_ASSERT(server != NULL, "Server initialization");

    mlink_property_list_t *props = mlink_property_list_create();
    mlink_property_list_add_string(props, "action", NULL);

    mlink_tool_t *tool = mlink_tool_create(
        "test.single",
        "Single tool test",
        props,
        dummy_callback,
        NULL,
        false);
    TEST_ASSERT(tool != NULL, "Tool creation");
    TEST_ASSERT(mlink_server_add_tool(server, tool), "Tool registration");

    mlink_property_list_destroy(props);
}

static void test_register_multiple_tools(void)
{
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "it.node.tool_reg.0");
    TEST_ASSERT(server != NULL, "Server initialization");

    /* 注册多个工具 */
    for (int i = 0; i < 5; i++) {
        char name[64];
        snprintf(name, sizeof(name), "test.tool%d", i);

        mlink_property_list_t *props = mlink_property_list_create();
        mlink_property_list_add_int(props, "value", false, 0, false, 0, false, 0);

        mlink_tool_t *tool = mlink_tool_create(
            name,
            "Test tool",
            props,
            dummy_callback,
            NULL,
            false);
        TEST_ASSERT(tool != NULL, "Tool creation");
        TEST_ASSERT(mlink_server_add_tool(server, tool), "Tool registration");

        mlink_property_list_destroy(props);
    }
}

static void test_register_duplicate_tool(void)
{
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "it.node.tool_reg.0");
    TEST_ASSERT(server != NULL, "Server initialization");

    mlink_property_list_t *props = mlink_property_list_create();
    mlink_property_list_add_string(props, "action", NULL);

    mlink_tool_t *tool1 = mlink_tool_create(
        "test.duplicate",
        "First tool",
        props,
        dummy_callback,
        NULL,
        false);
    TEST_ASSERT(tool1 != NULL, "First tool creation");
    TEST_ASSERT(mlink_server_add_tool(server, tool1), "First tool registration");

    mlink_tool_t *tool2 = mlink_tool_create(
        "test.duplicate",
        "Second tool",
        props,
        dummy_callback,
        NULL,
        false);
    TEST_ASSERT(tool2 != NULL, "Second tool creation");
    /* 重复注册应该失败 */
    bool ret = mlink_server_add_tool(server, tool2);
    TEST_ASSERT(!ret, "Duplicate tool registration should fail");

    mlink_tool_destroy(tool2);
    mlink_property_list_destroy(props);
}

static void test_register_tool_with_different_properties(void)
{
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "it.node.tool_reg.0");
    TEST_ASSERT(server != NULL, "Server initialization");

    /* 工具1：只有 int 参数 */
    mlink_property_list_t *props1 = mlink_property_list_create();
    mlink_property_list_add_int(props1, "value", false, 0, false, 0, false, 0);

    mlink_tool_t *tool1 = mlink_tool_create(
        "test.int_only",
        "Int parameter tool",
        props1,
        dummy_callback,
        NULL,
        false);
    TEST_ASSERT(mlink_server_add_tool(server, tool1), "Register int tool");
    mlink_property_list_destroy(props1);

    /* 工具2：只有 string 参数 */
    mlink_property_list_t *props2 = mlink_property_list_create();
    mlink_property_list_add_string(props2, "action", NULL);

    mlink_tool_t *tool2 = mlink_tool_create(
        "test.string_only",
        "String parameter tool",
        props2,
        dummy_callback,
        NULL,
        false);
    TEST_ASSERT(mlink_server_add_tool(server, tool2), "Register string tool");
    mlink_property_list_destroy(props2);

    /* 工具3：混合参数 */
    mlink_property_list_t *props3 = mlink_property_list_create();
    mlink_property_list_add_string(props3, "action", NULL);
    mlink_property_list_add_int(props3, "speed", true, 50, true, 0, true, 100);
    mlink_property_list_add_bool(props3, "enabled", true, true);

    mlink_tool_t *tool3 = mlink_tool_create(
        "test.mixed",
        "Mixed parameters tool",
        props3,
        dummy_callback,
        NULL,
        false);
    TEST_ASSERT(mlink_server_add_tool(server, tool3), "Register mixed tool");
    mlink_property_list_destroy(props3);
}

int main(void)
{
    printf("========================================\n");
    printf("   Tool Registration Integration Tests\n");
    printf("========================================\n\n");

    TEST_RUN(test_register_single_tool);
    TEST_RUN(test_register_multiple_tools);
    TEST_RUN(test_register_duplicate_tool);
    TEST_RUN(test_register_tool_with_different_properties);

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

