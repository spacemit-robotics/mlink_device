/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STORAGE_H
#define STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct storage;

struct storage *audio_storage_init(const char *path);

void storage_deinit(struct storage *storage);

void storage_enable(struct storage *storage, uint8_t enable);

int storage_write(struct storage *storage,
    void *data, size_t size);

void storage_enable(struct storage *storage, uint8_t enable);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* STORAGE_H */
