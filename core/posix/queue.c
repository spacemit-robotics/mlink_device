/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stddef.h>
#include <errno.h>

typedef struct queue {
    pthread_mutex_t lock;
    pthread_cond_t cond_not_empty;
    pthread_cond_t cond_not_full;
    char *buffer;
    unsigned int item_size;
    unsigned int queue_size;
    unsigned int head;
    unsigned int tail;
    volatile unsigned int count;
} queue_t;

struct queue *queue_create(const char *name, unsigned int item_size, unsigned int queue_size)
{
    (void)name;  /* 未使用的参数 */
    struct queue *q = malloc(sizeof(struct queue));
    if (!q) return NULL;

    q->item_size = item_size;
    q->queue_size = queue_size;
    q->buffer = malloc(item_size * queue_size);
    if (!q->buffer) {
        free(q);
        return NULL;
    }

    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond_not_empty, NULL);
    pthread_cond_init(&q->cond_not_full, NULL);
    q->head = q->tail = q->count = 0;
    return q;
}

void queue_delete(struct queue *queue)
{
    if (!queue) return;

    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->cond_not_empty);
    pthread_cond_destroy(&queue->cond_not_full);
    free(queue->buffer);
    free(queue);
}

int queue_send(struct queue *queue, void *buffer)
{
    if (!queue || !buffer) return -1;

    pthread_mutex_lock(&queue->lock);

    while (queue->count == queue->queue_size) {
        pthread_cond_wait(&queue->cond_not_full, &queue->lock);
    }

    memcpy(queue->buffer + queue->tail * queue->item_size,
            buffer, queue->item_size);

    queue->tail = (queue->tail + 1) % queue->queue_size;

    queue->count++;

    pthread_cond_signal(&queue->cond_not_empty);
    pthread_mutex_unlock(&queue->lock);
    return 0;
}

static void calc_timeout(struct timespec *ts, int timeout_ms)
{
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_sec += timeout_ms / 1000;
    ts->tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts->tv_nsec >= 1000000000) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000;
    }
}

int queue_recv(struct queue *queue, void *buffer, int timeout_ms)
{
    if (!queue || !buffer) return -1;

    int ret = 0;
    struct timespec ts;

    pthread_mutex_lock(&queue->lock);

    if (timeout_ms != 0) {
        if (timeout_ms > 0) calc_timeout(&ts, timeout_ms);

        while (queue->count == 0 && ret != ETIMEDOUT) {
            if (timeout_ms < 0) {
                pthread_cond_wait(&queue->cond_not_empty, &queue->lock);
            } else {
                ret = pthread_cond_timedwait(&queue->cond_not_empty,
                        &queue->lock, &ts);
                if (ret == ETIMEDOUT) break;
            }
        }
    }

    if (queue->count == 0) {
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }

    memcpy(buffer, queue->buffer + queue->head * queue->item_size, queue->item_size);
    queue->head = (queue->head + 1) % queue->queue_size;
    queue->count--;

    pthread_cond_signal(&queue->cond_not_full);
    pthread_mutex_unlock(&queue->lock);
    return 0;
}

int queue_reset(queue_t *queue)
{
    if (!queue) return -1;

    pthread_mutex_lock(&queue->lock);

    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;

    memset(queue->buffer, 0, queue->item_size * queue->queue_size);

    pthread_cond_broadcast(&queue->cond_not_empty);
    pthread_cond_broadcast(&queue->cond_not_full);

    pthread_mutex_unlock(&queue->lock);
    return 0;
}
