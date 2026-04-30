/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "background_sched.h"

void sample_task(void* arg) {
    int* val = (int*)arg;
    printf("Processing task %d\n", *val);
    free(val);
}

int main() {
    struct sched* sched = bg_sched_create();

    for (int i = 0; i < 5; ++i) {
        int* data = malloc(sizeof(int));
        *data = i;
        bg_sched_post(sched, sample_task, data);
    }

    sleep(5);

    bg_sched_destroy(sched);
    return 0;
}
