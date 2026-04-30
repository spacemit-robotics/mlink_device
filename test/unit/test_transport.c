/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
    * Transport 层单元测试
    */

#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include "mlink.h"
#include "utils/log.h"

static void test_transport_tcp_init(void)
{
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "unit.node.transport.0");
    TEST_ASSERT(server != NULL, "TCP transport initialization should succeed");
    /* 服务器是单例，无法直接销毁，这里只测试初始化 */
}

static void test_transport_types(void)
{
    /* 测试不同的传输类型（目前只实现 TCP） */
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "unit.node.transport.0");
    TEST_ASSERT(server != NULL, "TCP transport");

    /* 其他传输类型尚未实现，这里只测试 TCP */
}

static void test_transport_send_callback(void)
{
    mlink_server_t *server = mlink_server_init(TRANSPORT_TYPE_TCP, "unit.node.transport.0");
    TEST_ASSERT(server != NULL, "Server initialization");

    int callback_called = 0;
    void test_callback(const char *payload, void *ctx)
    {
        (void)payload;
        int *count = (int *)ctx;
        (*count)++;
    }

    mlink_server_set_send_callback(server, test_callback, &callback_called);

    /* 发送一个消息来触发 callback */
    const char *test_msg = "{\"jsonrpc\":\"2.0\",\"method\":\"initialize\",\"id\":1}";
    mlink_server_parse_message_str(server, test_msg);

    /* callback 应该被调用（至少一次，可能包括 hello 消息） */
    TEST_ASSERT(callback_called > 0, "Send callback should be called");
}

int main(void)
{
    printf("========================================\n");
    printf("   Transport Unit Tests\n");
    printf("========================================\n\n");

    TEST_RUN(test_transport_tcp_init);
    TEST_RUN(test_transport_types);
    TEST_RUN(test_transport_send_callback);

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

