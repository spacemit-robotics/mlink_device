/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUEUE_H
#define QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

struct queue;

struct queue *queue_create(const char *name,
    unsigned int item_size, unsigned int queue_size);

void queue_delete(struct queue *queue);

int queue_send(struct queue *queue, void *buffer);

int queue_recv(struct queue *queue, void *buffer, int timeout_ms);

int queue_reset(struct queue *queue);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* QUEUE_H */
