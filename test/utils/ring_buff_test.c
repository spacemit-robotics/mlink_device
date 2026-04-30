/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ringbuffer.h"

#define TEST_BUFFER_SIZE 1024

// 测试初始化状态
void test_init_state()
{
    struct ringbuff *rb = ringbuff_init(TEST_BUFFER_SIZE);
    assert(rb != NULL);

    // 验证初始状态
    assert(ringbuff_available(rb) == 0);
    assert(rb->read_pos == 0);
    assert(rb->write_pos == 0);

    ringbuff_deinit(rb);
    printf("[PASS] Init state test\n");
}

// 测试基本读写功能
void test_basic_rw()
{
    struct ringbuff *rb = ringbuff_init(TEST_BUFFER_SIZE);
    uint8_t test_data[512] = {0xAA};
    uint8_t read_buf[512] = {0};

    // 测试空读
    assert(ringbuff_read(rb, read_buf, 512) == 0);

    // 测试写入
    int written = ringbuff_write(rb, test_data, 512);
    assert(written == 512);
    assert(ringbuff_available(rb) == 512);

    // 测试读取
    int read = ringbuff_read(rb, read_buf, 512);
    assert(read == 512);
    assert(memcmp(test_data, read_buf, 512) == 0);
    assert(ringbuff_available(rb) == 0);

    ringbuff_deinit(rb);
    printf("[PASS] Basic RW test\n");
}

// 测试缓冲区循环特性
void test_wrap_around()
{
    struct ringbuff *rb = ringbuff_init(256);
    uint8_t data[200] = {0};

    // 第一次写入填满缓冲区
    assert(ringbuff_write(rb, data, 200) == 200);
    assert(ringbuff_write(rb, data, 100) == 56);  // 剩余空间56

    // 读取部分数据
    assert(ringbuff_read(rb, data, 150) == 150);

    // 测试循环写入
    assert(ringbuff_write(rb, data, 100) == 100);
    assert(ringbuff_available(rb) == 206);  // 256-56+100=206?

    ringbuff_deinit(rb);
    printf("[PASS] Wrap around test\n");
}

// 专项测试reset功能
void test_reset_function()
{
    struct ringbuff *rb = ringbuff_init(512);
    uint8_t data[256] = {0x55};

    // 写入数据后重置
    ringbuff_write(rb, data, 256);
    assert(ringbuff_reset(rb) == 0);

    // 验证重置状态
    assert(ringbuff_available(rb) == 0);
    assert(rb->read_pos == 0);
    assert(rb->write_pos == 0);

    // 验证缓冲区内容清零
    uint8_t read_buf[512] = {0};
    ringbuff_read(rb, read_buf, 512);  // 实际读取0字节
    assert(memcmp(read_buf, data, 256) != 0);  // 确认缓冲区被清零

    // 测试重置后正常使用
    assert(ringbuff_write(rb, data, 256) == 256);
    assert(ringbuff_read(rb, read_buf, 256) == 256);

    ringbuff_deinit(rb);
    printf("[PASS] Reset function test\n");
}

// 测试边界条件
void test_edge_cases()
{
    struct ringbuff *rb = ringbuff_init(128);

    // 测试满缓冲区
    assert(ringbuff_write(rb, (uint8_t[128]){0}, 128) == 128);
    assert(ringbuff_write(rb, (uint8_t[1]){0}, 1) == 0);

    // 测试空读取
    assert(ringbuff_read(rb, NULL, 128) == -1);  // 无效参数

    // 测试重置后的空操作
    ringbuff_reset(rb);
    assert(ringbuff_read(rb, (uint8_t[1]){0}, 1) == 0);

    ringbuff_deinit(rb);
    printf("[PASS] Edge cases test\n");
}

int main()
{
    test_init_state();
    test_basic_rw();
    test_wrap_around();
    test_reset_function();
    test_edge_cases();

    printf("All tests passed!\n");
    return 0;
}
