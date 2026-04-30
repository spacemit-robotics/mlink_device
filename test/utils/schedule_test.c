/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "schedule.h"

void my_task(void* arg)
{
    int* value = (int*)arg;
    printf("Task executed: %d\n", *value);
    free(value);
}

int main()
{
    printf("test start \n");
    struct schedule* sched = schedule_create();

    for (int i = 0; i < 5; i++) {
        int* data = malloc(sizeof(int));
        *data = i;
        schedule_post(sched, my_task, data);
    }

    printf("test run \n");

    schedule_run(sched, NULL);

    printf("test destroy \n");

    schedule_destroy(sched);

    printf("test end \n");
    return 0;
}
