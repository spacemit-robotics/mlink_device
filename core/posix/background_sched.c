/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "background_sched.h"
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

typedef struct task_node {
    bg_sched_cb callback;
    void* arg;
    struct task_node* next;
} task_node_t;

struct sched {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    task_node_t* queue_head;
    task_node_t* queue_tail;
    pthread_t worker;
    int running;
    size_t pending;
    size_t active;
};

static void* worker_main(void* arg)
{
    struct sched* s = (struct sched*)arg;
    while (1) {
        pthread_mutex_lock(&s->mutex);
        while (s->queue_head == NULL && s->running) {
            pthread_cond_wait(&s->cond, &s->mutex);
        }
        if (!s->running && s->queue_head == NULL) {
            pthread_mutex_unlock(&s->mutex);
            break;
        }

        task_node_t* task = s->queue_head;
        if (task) {
            s->queue_head = task->next;
            if (!s->queue_head) s->queue_tail = NULL;
            s->pending--;
            s->active++;
        }
        pthread_mutex_unlock(&s->mutex);

        if (task) {
            task->callback(task->arg);
            free(task);
            pthread_mutex_lock(&s->mutex);
            s->active--;
            pthread_mutex_unlock(&s->mutex);
        }
    }
    return NULL;
}

struct sched* bg_sched_create(const char *namefmt, int stacksize, int priority)
{
    struct sched* s = calloc(1, sizeof(struct sched));
    if (!s) return NULL;

    pthread_mutex_init(&s->mutex, NULL);
    pthread_cond_init(&s->cond, NULL);
    s->running = 1;
    pthread_attr_t attr;
    struct sched_param sched;
    pthread_attr_init(&attr);
    sched.sched_priority = priority;
    pthread_attr_setschedparam(&attr, &sched);
    if (stacksize > 0) {
        pthread_attr_setstacksize(&attr, stacksize * 8);
    }
    if (pthread_create(&s->worker, &attr, worker_main, s) != 0) {
        pthread_attr_destroy(&attr);
        pthread_mutex_destroy(&s->mutex);
        pthread_cond_destroy(&s->cond);
        free(s);
        return NULL;
    }
    pthread_attr_destroy(&attr);
    pthread_setname_np(s->worker, namefmt);
    return s;
}

int bg_sched_post(struct sched* s, bg_sched_cb fn, void* arg)
{
    if (!s || !fn) return -1;

    task_node_t* node = malloc(sizeof(task_node_t));
    if (!node) return -1;
    node->callback = fn;
    node->arg = arg;
    node->next = NULL;

    pthread_mutex_lock(&s->mutex);
    if (s->queue_tail) {
        s->queue_tail->next = node;
    } else {
        s->queue_head = node;
    }
    s->queue_tail = node;
    s->pending++;

    if (s->pending >= 30) {
        // check memery
    }
    pthread_cond_signal(&s->cond);
    pthread_mutex_unlock(&s->mutex);
    return 0;
}

void bg_sched_destroy(struct sched* s) {
    if (!s) return;

    pthread_mutex_lock(&s->mutex);
    s->running = 0;
    pthread_cond_broadcast(&s->cond);
    pthread_mutex_unlock(&s->mutex);

    pthread_join(s->worker, NULL);

    pthread_mutex_lock(&s->mutex);
    task_node_t* curr = s->queue_head;
    while (curr) {
        task_node_t* next = curr->next;
        free(curr);
        curr = next;
    }
    pthread_mutex_unlock(&s->mutex);

    pthread_mutex_destroy(&s->mutex);
    pthread_cond_destroy(&s->cond);
    free(s);
}
