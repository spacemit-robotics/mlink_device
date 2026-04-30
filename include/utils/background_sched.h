/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKGROUND_SCHED_H
#define BACKGROUND_SCHED_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct sched;

typedef void (*bg_sched_cb)(void* arg);

struct sched* bg_sched_create(const char *namefmt, int stacksize, int priority);

void bg_sched_destroy(struct sched* sched);

int bg_sched_post(struct sched* sched, bg_sched_cb fn, void* arg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BACKGROUND_SCHED_H */
