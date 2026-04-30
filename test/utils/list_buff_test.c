/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "list_buff.h"
#include <stdio.h>
#include <assert.h>

void print_packet(void *data, unsigned int len, void *user_data)
{
    printf("Packet[%s]: len=%u, data=%.*s\n",
            (const char *)user_data, len, len, (const char *)data);
}

void test_basic_operations()
{
    printf("=== Testing basic operations ===\n");
    struct list_buff *cache = list_buff_create();
    assert(cache);
    assert(list_buff_is_empty(cache));
    assert(list_buff_count(cache) == 0);

    // Test append and peek
    const char *pkt1 = "Packet1";
    assert(list_buff_append(cache, (void *)pkt1, strlen(pkt1), BUFF_STORE_REF) == 0);
    assert(!list_buff_is_empty(cache));
    assert(list_buff_count(cache) == 1);
    assert(list_buff_total_len(cache) == strlen(pkt1));

    void *data;
    unsigned int len;
    assert(list_buff_peek(cache, &data, &len) == 0);
    assert(data == pkt1);
    assert(len == strlen(pkt1));

    // Test pop
    assert(list_buff_pop(cache, &data, &len) == 0);
    assert(data == pkt1);
    assert(len == strlen(pkt1));
    assert(list_buff_is_empty(cache));

    // Test copy mode
    const char *pkt2 = "Packet2";
    assert(list_buff_append(cache, (void *)pkt2, strlen(pkt2), BUFF_STORE_COPY) == 0);
    assert(list_buff_peek(cache, &data, &len) == 0);
    assert(data != pkt2);  // Should be a copy
    assert(memcmp(data, pkt2, len) == 0);

    list_buff_destroy(cache, free);
    printf("Basic operations test passed!\n\n");
}

void test_foreach()
{
    printf("=== Testing foreach ===\n");
    struct list_buff *cache = list_buff_create();

    const char *packets[] = {"P1", "Packet2", "LongerPacket3"};
    for (int i = 0; i < 3; i++) {
        assert(list_buff_append(cache, (void *)packets[i], strlen(packets[i]),
                                i % 2 ? BUFF_STORE_REF : BUFF_STORE_COPY) == 0);
    }

    printf("All packets in cache:\n");
    list_buff_foreach(cache, print_packet, "Test");

    assert(list_buff_count(cache) == 3);
    list_buff_destroy(cache, free);
    printf("Foreach test passed!\n\n");
}

void test_clear()
{
    printf("=== Testing clear ===\n");
    struct list_buff *cache = list_buff_create();

    // Add both ref and copy packets
    char *pkt1 = strdup("DynamicPacket1");
    const char *pkt2 = "StaticPacket2";

    assert(list_buff_append(cache, pkt1, strlen(pkt1), BUFF_STORE_COPY) == 0);
    assert(list_buff_append(cache, (void *)pkt2, strlen(pkt2), BUFF_STORE_REF) == 0);

    // Clear should free copied packets but not referenced ones
    list_buff_clear(cache, free);
    assert(list_buff_is_empty(cache));

    // pkt1 was copied so original can be freed
    free(pkt1);

    list_buff_destroy(cache, free);
    printf("Clear test passed!\n\n");
}

int main()
{
    test_basic_operations();
    test_foreach();
    test_clear();
    printf("All tests passed!\n");
    return 0;
}
