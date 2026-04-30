/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils/list.h"
#include <stdlib.h>
#include <string.h>

void list_init(struct list *list, size_t item_size)
{
    if (!list) {
        return;
    }
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    list->item_size = item_size;
}

void list_free(struct list *list, list_free_func_t free_func)
{
    if (!list) {
        return;
    }

    if (free_func && list->items) {
        for (size_t i = 0; i < list->count; ++i) {
            void *item = (char *)list->items + i * list->item_size;
            free_func(item);
        }
    }

    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

bool list_reserve(struct list *list, size_t required)
{
    if (!list) {
        return false;
    }

    if (required <= list->capacity) {
        return true;
    }

    size_t new_capacity = list->capacity == 0 ? 4 : list->capacity * 2;
    while (new_capacity < required) {
        new_capacity *= 2;
    }

    void *new_items = realloc(list->items, new_capacity * list->item_size);
    if (!new_items) {
        return false;
    }

    list->items = new_items;
    list->capacity = new_capacity;
    return true;
}

bool list_add(struct list *list, const void *item, list_copy_func_t copy_func)
{
    if (!list || !item) {
        return false;
    }

    if (!list_reserve(list, list->count + 1)) {
        return false;
    }

    void *dst = (char *)list->items + list->count * list->item_size;

    if (copy_func) {
        copy_func(dst, item);
    } else {
        memcpy(dst, item, list->item_size);
    }

    list->count += 1;
    return true;
}

void *list_get(struct list *list, size_t index)
{
    if (!list || index >= list->count) {
        return NULL;
    }
    return (char *)list->items + index * list->item_size;
}

const void *list_get_const(const struct list *list, size_t index)
{
    if (!list || index >= list->count) {
        return NULL;
    }
    return (const char *)list->items + index * list->item_size;
}

size_t list_count(const struct list *list)
{
    return list ? list->count : 0;
}

size_t list_capacity(const struct list *list)
{
    return list ? list->capacity : 0;
}

void list_clear(struct list *list, list_free_func_t free_func)
{
    if (!list) {
        return;
    }

    if (free_func && list->items) {
        for (size_t i = 0; i < list->count; ++i) {
            void *item = (char *)list->items + i * list->item_size;
            free_func(item);
        }
    }

    list->count = 0;
}

bool list_clone(struct list *dst, const struct list *src, list_copy_func_t copy_func, list_free_func_t free_func)
{
    if (!dst || !src) {
        return false;
    }

    list_init(dst, src->item_size);

    for (size_t i = 0; i < src->count; ++i) {
        const void *src_item = list_get_const(src, i);
        if (!list_add(dst, src_item, copy_func)) {
            list_free(dst, free_func);
            return false;
        }
    }

    return true;
}

