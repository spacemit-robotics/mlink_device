/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TASK_H
#define TASK_H

#ifdef __cplusplus
extern "C" {
#endif

struct thread;

struct thread *thread_create_ex(void (*threadfn)(void *data), void *data,
    const char *namefmt, int stacksize, int priority, int detached);

struct thread *thread_create(void (*threadfn)(void *data),
    void *data, const char *namefmt, int stacksize, int priority);

int thread_stop_ex(struct thread *thread, int detached);

int thread_stop(struct thread *thread);

int thread_start(struct thread *thread);

int thread_msleep(unsigned int ms);

int thread_sleep(unsigned int s);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TASK_H */
