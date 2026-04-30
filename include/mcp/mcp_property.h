/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MCP_PROPERTY_H
#define MCP_PROPERTY_H

#include "mcp_server.h"
#include <stdbool.h>
#include <cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

void mcp_property_list_init(struct mcp_property_list *list);
void mcp_property_list_free(struct mcp_property_list *list);
bool mcp_property_list_add(struct mcp_property_list *list, const struct mcp_property *property);
struct mcp_property *mcp_property_list_get(struct mcp_property_list *list, const char *name);
const struct mcp_property *mcp_property_list_get_const(const struct mcp_property_list *list, const char *name);
bool mcp_property_set_bool(struct mcp_property *property, bool value);
bool mcp_property_set_int(struct mcp_property *property, int value);
bool mcp_property_set_string(struct mcp_property *property, const char *value);
struct mcp_property_list *mcp_property_list_create(void);
void mcp_property_list_destroy(struct mcp_property_list *list);
bool mcp_property_list_add_bool(struct mcp_property_list *list, const char *name, bool has_default, bool default_value);
bool mcp_property_list_add_int(struct mcp_property_list *list,
    const char *name,
    bool has_default,
    int default_value,
    bool has_min,
    int min_value,
    bool has_max,
    int max_value);
bool mcp_property_list_add_string(struct mcp_property_list *list, const char *name, const char *default_value);
bool mcp_property_list_get_bool(const struct mcp_property_list *list, const char *name, bool *out_value);
bool mcp_property_list_get_int(const struct mcp_property_list *list, const char *name, int *out_value);
const char *mcp_property_list_get_string(const struct mcp_property_list *list, const char *name);

bool mcp_property_copy_from_json(struct mcp_property *property, const cJSON *json);
char *mcp_property_to_json(const struct mcp_property *property);
bool mcp_property_append_json(cJSON *object, const struct mcp_property *property);
struct mcp_property_list mcp_property_list_clone(const struct mcp_property_list *source);

/* Internal macro: convert mcp_property_list to struct list. */
#define PROPERTY_LIST_TO_LIST(plist) (&(plist)->base)

#ifdef __cplusplus
}
#endif

#endif /* MCP_PROPERTY_H */

