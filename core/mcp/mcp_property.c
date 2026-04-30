/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mcp/mcp_property.h"
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>
#include "utils/utils.h"
#include "utils/list.h"

/* Property复制函数，用于通用list */
static void property_copy_func(void *dst, const void *src)
{
    struct mcp_property *d = (struct mcp_property *)dst;
    const struct mcp_property *s = (const struct mcp_property *)src;

    memset(d, 0, sizeof(*d));
    d->type = s->type;
    d->has_default = s->has_default;
    d->has_min = s->has_min;
    d->has_max = s->has_max;
    d->min_value = s->min_value;
    d->max_value = s->max_value;
    d->name = utils_strdup(s->name);
    if (s->type == MCP_PROPERTY_STRING && s->has_default) {
        d->value.string_value = utils_strdup(s->value.string_value);
    } else {
        d->value = s->value;
    }
}

/* Property释放函数，用于通用list */
static void property_free_func(void *item)
{
    struct mcp_property *property = (struct mcp_property *)item;
    if (!property) {
        return;
    }
    free(property->name);
    if (property->type == MCP_PROPERTY_STRING && property->has_default) {
        free(property->value.string_value);
    }
    memset(property, 0, sizeof(*property));
}

void mcp_property_list_init(struct mcp_property_list *list)
{
    if (!list) {
        return;
    }
    list_init(PROPERTY_LIST_TO_LIST(list), sizeof(struct mcp_property));
}

void mcp_property_list_free(struct mcp_property_list *list)
{
    if (!list) {
        return;
    }
    list_free(PROPERTY_LIST_TO_LIST(list), property_free_func);
}

bool mcp_property_list_add(struct mcp_property_list *list, const struct mcp_property *property)
{
    if (!list || !property) {
        return false;
    }
    return list_add(PROPERTY_LIST_TO_LIST(list), property, property_copy_func);
}

struct mcp_property *mcp_property_list_get(struct mcp_property_list *list, const char *name)
{
    if (!list || !name) {
        return NULL;
    }
    const struct list *l = PROPERTY_LIST_TO_LIST(list);
    size_t count = list_count(l);
    for (size_t i = 0; i < count; ++i) {
        struct mcp_property *property = (struct mcp_property *)list_get((struct list *)l, i);
        if (property && property->name && strcmp(property->name, name) == 0) {
            return property;
        }
    }
    return NULL;
}

const struct mcp_property *mcp_property_list_get_const(const struct mcp_property_list *list, const char *name)
{
    if (!list || !name) {
        return NULL;
    }
    const struct list *l = PROPERTY_LIST_TO_LIST((struct mcp_property_list *)list);
    size_t count = list_count(l);
    for (size_t i = 0; i < count; ++i) {
        const struct mcp_property *property = (const struct mcp_property *)list_get_const(l, i);
        if (property && property->name && strcmp(property->name, name) == 0) {
            return property;
        }
    }
    return NULL;
}

bool mcp_property_set_bool(struct mcp_property *property, bool value)
{
    if (!property || property->type != MCP_PROPERTY_BOOLEAN) {
        return false;
    }
    property->value.bool_value = value;
    property->has_default = true;
    return true;
}

bool mcp_property_set_int(struct mcp_property *property, int value)
{
    if (!property || property->type != MCP_PROPERTY_INTEGER) {
        return false;
    }
    if (property->has_min && value < property->min_value) {
        return false;
    }
    if (property->has_max && value > property->max_value) {
        return false;
    }
    property->value.int_value = value;
    property->has_default = true;
    return true;
}

bool mcp_property_set_string(struct mcp_property *property, const char *value)
{
    if (!property || property->type != MCP_PROPERTY_STRING) {
        return false;
    }
    free(property->value.string_value);
    property->value.string_value = utils_strdup(value ? value : "");
    property->has_default = true;
    return property->value.string_value != NULL;
}

struct mcp_property_list *mcp_property_list_create(void)
{
    struct mcp_property_list *list = (struct mcp_property_list *)malloc(sizeof(struct mcp_property_list));
    if (!list) {
        return NULL;
    }
    mcp_property_list_init(list);
    return list;
}

void mcp_property_list_destroy(struct mcp_property_list *list)
{
    if (!list) {
        return;
    }
    mcp_property_list_free(list);
    free(list);
}

bool mcp_property_list_add_bool(struct mcp_property_list *list, const char *name, bool has_default, bool default_value)
{
    if (!list || !name) {
        return false;
    }
    struct mcp_property property;
    memset(&property, 0, sizeof(property));
    property.name = utils_strdup(name);
    if (!property.name) {
        return false;
    }
    property.type = MCP_PROPERTY_BOOLEAN;
    property.has_default = has_default;
    property.value.bool_value = default_value;
    bool ok = mcp_property_list_add(list, &property);
    property_free_func(&property);
    return ok;
}

bool mcp_property_list_add_int(struct mcp_property_list *list,
                                const char *name,
                                bool has_default,
                                int default_value,
                                bool has_min,
                                int min_value,
                                bool has_max,
                                int max_value)
{
    if (!list || !name) {
        return false;
    }
    struct mcp_property property;
    memset(&property, 0, sizeof(property));
    property.name = utils_strdup(name);
    if (!property.name) {
        return false;
    }
    property.type = MCP_PROPERTY_INTEGER;
    property.has_default = has_default;
    property.value.int_value = default_value;
    property.has_min = has_min;
    property.min_value = min_value;
    property.has_max = has_max;
    property.max_value = max_value;
    bool ok = mcp_property_list_add(list, &property);
    property_free_func(&property);
    return ok;
}

bool mcp_property_list_add_string(struct mcp_property_list *list, const char *name, const char *default_value)
{
    if (!list || !name) {
        return false;
    }
    struct mcp_property property;
    memset(&property, 0, sizeof(property));
    property.name = utils_strdup(name);
    if (!property.name) {
        return false;
    }
    property.type = MCP_PROPERTY_STRING;
    if (default_value) {
        property.has_default = true;
        property.value.string_value = utils_strdup(default_value);
        if (!property.value.string_value) {
            property_free_func(&property);
            return false;
        }
    }
    bool ok = mcp_property_list_add(list, &property);
    property_free_func(&property);
    return ok;
}

bool mcp_property_list_get_bool(const struct mcp_property_list *list, const char *name, bool *out_value)
{
    if (!list || !name) {
        return false;
    }
    const struct mcp_property *property = mcp_property_list_get_const(list, name);
    if (!property || property->type != MCP_PROPERTY_BOOLEAN || !property->has_default) {
        return false;
    }
    if (out_value) {
        *out_value = property->value.bool_value;
    }
    return true;
}

bool mcp_property_list_get_int(const struct mcp_property_list *list, const char *name, int *out_value)
{
    if (!list || !name) {
        return false;
    }
    const struct mcp_property *property = mcp_property_list_get_const(list, name);
    if (!property || property->type != MCP_PROPERTY_INTEGER || !property->has_default) {
        return false;
    }
    if (out_value) {
        *out_value = property->value.int_value;
    }
    return true;
}

const char *mcp_property_list_get_string(const struct mcp_property_list *list, const char *name)
{
    if (!list || !name) {
        return NULL;
    }
    const struct mcp_property *property = mcp_property_list_get_const(list, name);
    if (!property || property->type != MCP_PROPERTY_STRING || !property->has_default) {
        return NULL;
    }
    return property->value.string_value;
}

bool mcp_property_copy_from_json(struct mcp_property *property, const cJSON *json)
{
    if (!property || !json) {
        return false;
    }
    switch (property->type) {
        case MCP_PROPERTY_BOOLEAN:
            if (cJSON_IsBool(json)) {
                return mcp_property_set_bool(property, json->valueint == 1);
            }
            break;
        case MCP_PROPERTY_INTEGER:
            if (cJSON_IsNumber(json)) {
                return mcp_property_set_int(property, json->valueint);
            }
            break;
        case MCP_PROPERTY_STRING:
            if (cJSON_IsString(json)) {
                return mcp_property_set_string(property, json->valuestring);
            }
            break;
        default:
            break;
    }
    return false;
}

char *mcp_property_to_json(const struct mcp_property *property)
{
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        return NULL;
    }
    switch (property->type) {
        case MCP_PROPERTY_BOOLEAN:
            cJSON_AddStringToObject(json, "type", "boolean");
            if (property->has_default) {
                cJSON_AddBoolToObject(json, "default", property->value.bool_value);
            }
            break;
        case MCP_PROPERTY_INTEGER:
            cJSON_AddStringToObject(json, "type", "integer");
            if (property->has_default) {
                cJSON_AddNumberToObject(json, "default", property->value.int_value);
            }
            if (property->has_min) {
                cJSON_AddNumberToObject(json, "minimum", property->min_value);
            }
            if (property->has_max) {
                cJSON_AddNumberToObject(json, "maximum", property->max_value);
            }
            break;
        case MCP_PROPERTY_STRING:
            cJSON_AddStringToObject(json, "type", "string");
            if (property->has_default && property->value.string_value) {
                cJSON_AddStringToObject(json, "default", property->value.string_value);
            }
            break;
        default:
            break;
    }
    char *result = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    return result;
}

bool mcp_property_append_json(cJSON *object, const struct mcp_property *property)
{
    char *property_json = mcp_property_to_json(property);
    if (!property_json) {
        return false;
    }
    cJSON *parsed = cJSON_Parse(property_json);
    free(property_json);
    if (!parsed) {
        return false;
    }
    cJSON_AddItemToObject(object, property->name, parsed);
    return true;
}

struct mcp_property_list mcp_property_list_clone(const struct mcp_property_list *source)
{
    struct mcp_property_list clone;
    mcp_property_list_init(&clone);
    const struct list *src_list = PROPERTY_LIST_TO_LIST((struct mcp_property_list *)source);
    size_t count = list_count(src_list);
    for (size_t i = 0; i < count; ++i) {
        const struct mcp_property *property = (const struct mcp_property *)list_get_const(src_list, i);
        if (!mcp_property_list_add(&clone, property)) {
            mcp_property_list_free(&clone);
            break;
        }
    }
    return clone;
}

