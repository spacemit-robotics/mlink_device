/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
    * 服务器初始化集成测试
    */

#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include "mlink.h"

static void test_server_init_tcp(void)
{
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "it.node.server_init.0");
    TEST_ASSERT(server != NULL, "Server initialization with TCP should succeed");
}

static void test_server_init_multiple_times(void)
{
    /* 测试多次初始化（应该是单例） */
    mlink_server_t *server1 = mlink_server_init(TRANSPORT_TYPE_TCP, "it.node.server_init.0");
    mlink_server_t *server2 = mlink_server_init(TRANSPORT_TYPE_TCP, "it.node.server_init.0");

    TEST_ASSERT(server1 != NULL, "First initialization");
    TEST_ASSERT(server2 != NULL, "Second initialization");
    /* 单例模式，应该返回同一个实例 */
    TEST_ASSERT(server1 == server2, "Should return same instance (singleton)");
}

static void test_server_set_callback(void)
{
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "it.node.server_init.0");
    TEST_ASSERT(server != NULL, "Server initialization");

    void test_callback(const char *payload, void *ctx)
    {
        (void)payload;
        (void)ctx;
    }

    mlink_server_set_send_callback(server, test_callback, NULL);
    /* 设置应该成功，没有返回值验证 */
}

static void test_server_init_with_tools(void)
{
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "it.node.server_init.0");
    TEST_ASSERT(server != NULL, "Server initialization");

    /* 创建并注册工具 */
    mlink_property_list_t *props = mlink_property_list_create();
    mlink_property_list_add_int(props, "value", false, 0, false, 0, false, 0);

    struct mlink_return_value tool_cb(const mlink_property_list_t *p, void *ctx)
    {
        (void)p;
        (void)ctx;
        return mlink_return_bool(true);
    }

    mlink_tool_t *tool = mlink_tool_create(
        "test.init_tool",
        "Test tool for init",
        props,
        tool_cb,
        NULL,
        false);
    TEST_ASSERT(tool != NULL, "Tool creation");
    TEST_ASSERT(mlink_server_add_tool(server, tool), "Tool registration");

    mlink_property_list_destroy(props);
}

int main(void)
{
    printf("========================================\n");
    printf("   Server Initialization Integration Tests\n");
    printf("========================================\n\n");

    TEST_RUN(test_server_init_tcp);
    TEST_RUN(test_server_init_multiple_times);
    TEST_RUN(test_server_set_callback);
    TEST_RUN(test_server_init_with_tools);

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

