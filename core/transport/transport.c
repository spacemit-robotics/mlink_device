/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transport.h"
#include <stdlib.h>
#include <string.h>

/* 目前使用到 TCP 与 UNIX 两种传输类型，预留更多插槽便于扩展 */
#define MAX_TRANSPORT_REGISTRATIONS 4

struct transport {
    const char *name;
    struct hwio_ops *ops;
    struct hwio hw;   /* 每个 transport 拥有独立的 hwio 上下文 */
};

struct transport_registry {
    enum transport_type type;
    struct hwio_ops *ops;
};

/* 传输类型注册表 */
static struct transport_registry transport_table[MAX_TRANSPORT_REGISTRATIONS];

static size_t transport_table_count = 0;

/* 注册传输类型的实现 */
int transport_register(enum transport_type type, void *ops)
{
    if (!ops) {
        return -1;
    }

    /* 检查是否已注册 */
    for (size_t i = 0; i < transport_table_count; i++) {
        if (transport_table[i].type == type) {
            transport_table[i].ops = (struct hwio_ops *)ops;
            return 0; /* 更新已存在的注册 */
        }
    }

    /* 检查注册表是否已满 */
    if (transport_table_count >= MAX_TRANSPORT_REGISTRATIONS) {
        return -1;
    }

    /* 添加新注册 */
    transport_table[transport_table_count].type = type;
    transport_table[transport_table_count].ops = (struct hwio_ops *)ops;
    transport_table_count++;

    return 0;
}

/* 注销传输类型 */
int transport_unregister(enum transport_type type)
{
    for (size_t i = 0; i < transport_table_count; i++) {
        if (transport_table[i].type == type) {
            /* 将后面的元素前移 */
            for (size_t j = i; j < transport_table_count - 1; j++) {
                transport_table[j] = transport_table[j + 1];
            }
            transport_table_count--;
            return 0;
        }
    }
    return -1;
}

struct transport *transport_create(enum transport_type type, const char *name)
{
    struct transport *transport = malloc(sizeof(struct transport));
    if (!transport) return NULL;
    transport->name = name;
    transport->ops = NULL;
    memset(&transport->hw, 0, sizeof(struct hwio));

    /* 从注册表中查找对应的操作结构 */
    for (size_t i = 0; i < transport_table_count; i++) {
        if (transport_table[i].type == type) {
            transport->ops = (struct hwio_ops *)transport_table[i].ops;
            break;
        }
    }

    if (!transport->ops) {
        free(transport);
        return NULL;
    }

    /* 调用底层实现的 init（如果有），用于启动接收线程等 */
    if (transport->ops->init) {
        if (transport->ops->init(&transport->hw) != 0) {
            free(transport);
            return NULL;
        }
    }

    return transport;
}

void transport_destroy(struct transport *transport)
{
    if (!transport) return;
    /* 通知底层实现做清理 */
    if (transport->ops && transport->ops->deinit) {
        transport->ops->deinit(&transport->hw);
    }
    free(transport);
}

int transport_send(struct transport *transport, const void *data, uint32_t size)
{
    if (!transport || !transport->ops || !transport->ops->write) return -1;
    return transport->ops->write(&transport->hw, data, size);
}

int transport_register_callback(struct transport *transport,
        hwio_callback_t callback, void *user_ctx)
{
    if (!transport || !transport->ops || !transport->ops->callback) return -1;
    return transport->ops->callback(&transport->hw, callback, user_ctx);
}
