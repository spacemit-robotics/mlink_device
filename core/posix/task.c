/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

struct thread {
    pthread_t tid;
    void (*threadfn)(void *data);
    void *data;
};

struct thread *thread_create_ex(void (*threadfn)(void *data), void *data,
                            const char *namefmt, int stacksize, int priority, int detached)
{
    struct thread* thread = malloc(sizeof(struct thread));
    if (!thread) return NULL;

    thread->threadfn = threadfn;
    thread->data = data;

    pthread_attr_t attr;
    struct sched_param sched;
    sched.sched_priority = priority;
    pthread_attr_init(&attr);
    pthread_attr_setschedparam(&attr, &sched);
    if (stacksize > 0) {
        pthread_attr_setstacksize(&attr, stacksize * 8);
    }
    if (detached) {
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }
    /* 创建线程，将 void (*)(void*) 转换为 void* (*)(void*) */
    void* (*wrapper)(void*) = (void* (*)(void*))(void*)threadfn;
    if (pthread_create(&thread->tid, &attr, wrapper, data) != 0) {
        free(thread);
        pthread_attr_destroy(&attr);
        return NULL;
    }

    pthread_attr_destroy(&attr);
    if (namefmt)
        pthread_setname_np(thread->tid, namefmt);
    return thread;
}

struct thread *thread_create(void (*threadfn)(void *data), void *data,
                    const char *namefmt, int stacksize, int priority)
{
    return thread_create_ex(threadfn, data, namefmt, stacksize, priority, 0);
}

int thread_stop_ex(struct thread *thread, int detached)
{
    if (!thread)
        return 0;

    if (!detached) {
        pthread_cancel(thread->tid);
        pthread_join(thread->tid, NULL);
    }

    free(thread);

    return 0;
}

int thread_stop(struct thread *thread)
{
    return thread_stop_ex(thread, 0);
}

int thread_msleep(unsigned int ms)
{
    return usleep(1000 * ms);
}

int thread_sleep(unsigned int sec)
{
    return sleep(sec);
}
