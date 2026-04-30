/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "task.h"

void demo_thread(void* data)
{
    while (1) {
        printf("Working...\n");
        thread_msleep(500);
    }
}

int main() {
    struct thread *t = thread_create(demo_thread, NULL, "demo", 2048, 5);

    thread_sleep(2);

    thread_stop(t);

    return 0;
}
