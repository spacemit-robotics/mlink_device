/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mutex.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define THREAD_NUM 2
static int shared_counter = 0;
static struct ch_mutex *counter_mutex;

void *thread_func(void *arg)
{
    for (int i = 0; i < 100000; ++i) {
        mutex_lock(counter_mutex);
        shared_counter++;
        mutex_unlock(counter_mutex);
    }
    return NULL;
}

int main()
{
    counter_mutex = mutex_init();
    if (!counter_mutex) {
        fprintf(stderr, "Mutex init failed\n");
        return 1;
    }

    pthread_t threads[THREAD_NUM];
    for (int i = 0; i < THREAD_NUM; ++i) {
        if (pthread_create(&threads[i], NULL, thread_func, NULL) != 0) {
            fprintf(stderr, "Thread create failed\n");
            return 1;
        }
    }

    for (int i = 0; i < THREAD_NUM; ++i) {
        pthread_join(threads[i], NULL);
    }

    printf("Expected value: %d\n", THREAD_NUM * 100000);
    printf("Actual value: %d\n", shared_counter);

    mutex_deinit(counter_mutex);
    return 0;
}
