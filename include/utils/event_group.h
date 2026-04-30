/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EVENT_GROUP_H
#define EVENT_GROUP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct event_group;

struct event_group *event_group_create(void);

void event_group_set_bit(struct event_group *evt, unsigned char event_bit);

void event_group_clear_bit(struct event_group *evt, unsigned char event_bit);

unsigned char event_group_wait(struct event_group* eg, unsigned char wait_bits,
    unsigned char clear_bits, int wait_all, unsigned int timeout_ms);

unsigned char event_group_get_bit(struct event_group *evt, unsigned char event_bit);

void event_group_delete(struct event_group *evt);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* EVENT_GROUP_H */
