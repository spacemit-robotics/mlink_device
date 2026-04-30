/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Generic dynamic array structure. */
struct list {
    void *items;           /* Element array. */
    size_t count;          /* Current element count. */
    size_t capacity;       /* Capacity. */
    size_t item_size;      /* Size of each element. */
};

/* Element copy function type. */
typedef void (*list_copy_func_t)(void *dst, const void *src);

/* Element free function type. */
typedef void (*list_free_func_t)(void *item);

/* Initialize list. */
void list_init(struct list *list, size_t item_size);

/* Free list using a custom free function. */
void list_free(struct list *list, list_free_func_t free_func);

/* Reserve capacity. */
bool list_reserve(struct list *list, size_t required);

/* Add element using a custom copy function. */
bool list_add(struct list *list, const void *item, list_copy_func_t copy_func);

/* Get element by index. */
void *list_get(struct list *list, size_t index);

/* Get const pointer to element by index. */
const void *list_get_const(const struct list *list, size_t index);

/* Get element count. */
size_t list_count(const struct list *list);

/* Get capacity. */
size_t list_capacity(const struct list *list);

/* Clear list and free each element. */
void list_clear(struct list *list, list_free_func_t free_func);

/* Clone list. */
bool list_clone(struct list *dst, const struct list *src, list_copy_func_t copy_func, list_free_func_t free_func);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LIST_H */

