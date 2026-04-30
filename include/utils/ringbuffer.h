/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RINGBUFFER_H
#define RINGBUFFER_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

struct ringbuff {
    uint8_t *buffer;
    uint32_t size;
    uint32_t read_pos;
    uint32_t write_pos;
    struct ch_mutex *lock;
    uint32_t data_len;
};

struct ringbuff *ringbuff_init(uint32_t size);

void ringbuff_deinit(struct ringbuff *rb);

int ringbuff_write(struct ringbuff *rb, const uint8_t *data, uint32_t len);

int ringbuff_read(struct ringbuff *rb, uint8_t *data, uint32_t len);

int ringbuff_reset(struct ringbuff *rb);

uint32_t ringbuff_available(struct ringbuff *rb);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RINGBUFFER_H */
