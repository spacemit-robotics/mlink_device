/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mutex.h"
#include <stdlib.h>
#include <pthread.h>

struct ch_mutex {
    pthread_mutex_t pmutex;
};

struct ch_mutex *mutex_init(void)
{
    struct ch_mutex *m = malloc(sizeof(*m));
    if (!m) return NULL;

    if (pthread_mutex_init(&m->pmutex, NULL) != 0) {
        free(m);
        return NULL;
    }
    return m;
}

int mutex_deinit(struct ch_mutex *mutex)
{
    if (!mutex) return -1;

    pthread_mutex_destroy(&mutex->pmutex);
    free(mutex);
    return 0;
}

int mutex_lock(struct ch_mutex *mutex)
{
    if (!mutex) return -1;
    return pthread_mutex_lock(&mutex->pmutex);
}

int mutex_unlock(struct ch_mutex *mutex)
{
    if (!mutex) return -1;
    return pthread_mutex_unlock(&mutex->pmutex);
}
