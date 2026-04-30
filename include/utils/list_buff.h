/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIST_BUFF_H
#define LIST_BUFF_H

#include <stdlib.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif


enum buff_store_mode {
    BUFF_STORE_COPY,
    BUFF_STORE_REF
};


struct list_buff;

struct list_buff *list_buff_create(void);
void list_buff_destroy(struct list_buff *cache, void (*free_buff)(void *));
int list_buff_append(struct list_buff *cache, void *buff_data, unsigned int buff_len, enum buff_store_mode mode);
int list_buff_pop(struct list_buff *cache, void **buff_data, unsigned int *buff_len, int free_buff);
void list_buff_free_pop_buff(void *buff_data);
int list_buff_peek(struct list_buff *cache, void **buff_data, unsigned int *buff_len);
unsigned int list_buff_count(struct list_buff *cache);
unsigned int list_buff_total_len(struct list_buff *cache);
int list_buff_is_empty(struct list_buff *cache);
void list_buff_clear(struct list_buff *cache, void (*free_buff)(void *));
void list_buff_foreach(
    struct list_buff *cache,
    void (*callback)(void *buff_data, unsigned int buff_len, void *user_data),
    void *user_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LIST_BUFF_H */
