/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "schedule.h"
#include <stdlib.h>
#include <pthread.h>

struct schedule {
    pthread_mutex_t lock;
    struct task_node* task_queue;
    volatile int running;
};

struct task_node {
    schedule_callback_t callback;
    void* arg;
    struct task_node* next;
};

struct schedule* schedule_create(void)
{
    struct schedule* sched = malloc(sizeof(struct schedule));
    if (!sched) return NULL;

    pthread_mutex_init(&sched->lock, NULL);
    sched->task_queue = NULL;
    sched->running = 1;
    return sched;
}

void schedule_destroy(struct schedule* sched)
{
    if (!sched) return;

    pthread_mutex_lock(&sched->lock);
    sched->running = 0;

    struct task_node* task = sched->task_queue;
    while (task) {
        struct task_node* next = task->next;
        free(task);
        task = next;
    }
    pthread_mutex_unlock(&sched->lock);

    pthread_mutex_destroy(&sched->lock);
    free(sched);
}

int schedule_post(struct schedule* sched, schedule_callback_t fn, void* arg)
{
    if (!sched || !fn) return -1;

    struct task_node* new_task = malloc(sizeof(struct task_node));

    if (!new_task) return -1;
    new_task->callback = fn;
    new_task->arg = arg;
    new_task->next = NULL;

    pthread_mutex_lock(&sched->lock);
    if (!sched->task_queue) {
        sched->task_queue = new_task;
    } else {
        struct task_node* tail = sched->task_queue;
        while (tail->next) tail = tail->next;
        tail->next = new_task;
    }

    pthread_mutex_unlock(&sched->lock);
    return 0;
}

void schedule_run(struct schedule* sched, void* user_data)
{
    (void)user_data;  /* 未使用的参数 */
    struct task_node* task = NULL;

    if (!sched) return;
    while (sched->running && sched->task_queue) {
        pthread_mutex_lock(&sched->lock);

        if (!sched->running) {
            pthread_mutex_unlock(&sched->lock);
            break;
        }

        task = sched->task_queue;

        sched->task_queue = task->next;
        pthread_mutex_unlock(&sched->lock);

        if (task->callback) {
            task->callback(task->arg);
        }
        free(task);
    }
}
