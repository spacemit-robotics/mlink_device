/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include "event_group.h"

#define BITS_MASK 0xFF

struct event_group {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    unsigned char bits;
};

struct event_group* event_group_create(void)
{
    struct event_group* eg = malloc(sizeof(*eg));
    if (!eg) return NULL;

    if (pthread_mutex_init(&eg->mutex, NULL) != 0) {
        free(eg);
        return NULL;
    }

    if (pthread_cond_init(&eg->cond, NULL) != 0) {
        pthread_mutex_destroy(&eg->mutex);
        free(eg);
        return NULL;
    }

    eg->bits = 0;
    return eg;
}

void event_group_set_bit(struct event_group* eg, unsigned char event_bit)
{
    if (!eg || (event_bit & ~BITS_MASK)) return;

    pthread_mutex_lock(&eg->mutex);
    eg->bits |= (event_bit & BITS_MASK);
    pthread_cond_broadcast(&eg->cond);
    pthread_mutex_unlock(&eg->mutex);
}

void event_group_clear_bit(struct event_group* eg, unsigned char event_bit)
{
    if (!eg || (event_bit & ~BITS_MASK)) return;

    pthread_mutex_lock(&eg->mutex);
    eg->bits &= ~(event_bit & BITS_MASK);
    pthread_mutex_unlock(&eg->mutex);
}
unsigned char event_group_wait(struct event_group* eg, unsigned char wait_bits,
    unsigned char clear_bits, int wait_all, unsigned int timeout_ms)
{
    if (!eg || (wait_bits & ~BITS_MASK)) return 0;

    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += timeout_ms / 1000;
    timeout.tv_nsec += (timeout_ms % 1000) * 1000000L;


    if (timeout.tv_nsec >= 1000000000L) {
        timeout.tv_sec += 1;
        timeout.tv_nsec -= 1000000000L;
    }

    pthread_mutex_lock(&eg->mutex);

    while (1) {
        const unsigned char current = eg->bits & BITS_MASK;
        const unsigned char matched = current & wait_bits;

        const int condition_met = wait_all ?
            (matched == (wait_bits & BITS_MASK)) :
            (matched != 0);

        if (condition_met) {
            if (clear_bits != 0) {
                eg->bits &= ~(clear_bits & BITS_MASK);
            }
            pthread_mutex_unlock(&eg->mutex);
            return matched;
        }

        if (timeout_ms == 0) {
            pthread_mutex_unlock(&eg->mutex);
            return current & wait_bits;
        }

        const int wait_result = pthread_cond_timedwait(&eg->cond, &eg->mutex, &timeout);
        if (wait_result == ETIMEDOUT) {
            const unsigned char remaining = eg->bits & wait_bits;
            pthread_mutex_unlock(&eg->mutex);
            return remaining;
        }
    }
}
unsigned char event_group_get_bit(struct event_group *eg, unsigned char event_bit)
{
    if (!eg || (event_bit & 0x80)) {
        return 0;
    }

    pthread_mutex_lock(&eg->mutex);
    const unsigned char value = eg->bits & event_bit;
    pthread_mutex_unlock(&eg->mutex);

    return value;
}
void event_group_delete(struct event_group* eg)
{
    if (!eg) return;

    pthread_mutex_destroy(&eg->mutex);
    pthread_cond_destroy(&eg->cond);
    free(eg);
}
