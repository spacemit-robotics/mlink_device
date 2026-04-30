/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HWIO_H
#define HWIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

struct hwio {
    void *priv;  /* Implementation-specific context pointer for underlying transport. */
};

typedef void (*hwio_callback_t)(const void *data, size_t size, void *ctx);

struct hwio_ops {
    int (*init)(struct hwio *hwio);
    int (*deinit)(struct hwio *hwio);
    int (*write)(struct hwio *hwio, const void *data, size_t size);
    int (*callback)(struct hwio *hwio, hwio_callback_t callback, void *ctx);
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HWIO_H */
