/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TRANSPORT_H
#define TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "transport/hwio.h"
#include "mlink.h"

struct transport;

typedef void (*transport_callback_t)(const void *data, uint32_t size, void *ctx);

/* Register / unregister transport type implementations. */
int transport_register(enum transport_type type, void *ops);
int transport_unregister(enum transport_type type);

struct transport *transport_create(enum transport_type type, const char *name);

void transport_destroy(struct transport *transport);

int transport_send(struct transport *transport, const void *data, uint32_t size);

int transport_register_callback(struct transport *transport,
        hwio_callback_t callback, void *user_ctx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TRANSPORT_H */
