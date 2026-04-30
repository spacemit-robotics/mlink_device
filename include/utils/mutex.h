/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MUTEX_H
#define MUTEX_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct ch_mutex;

struct ch_mutex *mutex_init(void);

int mutex_deinit(struct ch_mutex *mutex);

int mutex_lock(struct ch_mutex *mutex);

int mutex_unlock(struct ch_mutex *mutex);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MUTEX_H */
