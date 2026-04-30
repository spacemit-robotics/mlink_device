/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SCHEDULE_H
#define SCHEDULE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct schedule;

typedef void (*schedule_callback_t)(void* arg);

struct schedule* schedule_create(void);
void schedule_destroy(struct schedule* sched);
int schedule_post(struct schedule* sched, schedule_callback_t fn, void* arg);
void schedule_run(struct schedule* sched, void* user_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SCHEDULE_H */
