/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <unistd.h>
#include "sys_time.h"

int main()
{
    uint32_t start = sys_time_now();
    sleep(1);
    uint32_t end = sys_time_now();

    printf("Elapsed: %ums\n", end - start);
    return 0;
}
