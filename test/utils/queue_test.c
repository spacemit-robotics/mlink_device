/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "queue.h"
#include "task.h"

static queue_t *q;

void producer(void *arg)
{
    int data = 0;
    while (1) {
        queue_send(q, &data);
        data++;
        thread_sleep(1);
    }
}

void consumer(void *arg)
{
    int data;
    while (1) {
        if (queue_recv(q, &data, 100) == 0) {
            printf("Received: %d\n", data);
        }
    }
}

int main(int argc, char *argv[])
{
    q = queue_create("test", sizeof(int), 20);

    struct thread *t1 = thread_create(producer, NULL, "tx", 2048, 5);
    struct thread *t2 = thread_create(consumer, NULL, "rx", 2048, 5);

    thread_sleep(10);

    thread_stop(t1);
    thread_stop(t2);

    queue_delete(q);

    return 0;
}
