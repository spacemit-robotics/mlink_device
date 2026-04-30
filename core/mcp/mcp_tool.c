/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mcp/mcp_tool.h"
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>
#include "mcp/mcp_property.h"
#include "mcp/mcp_return_value.h"
#include "utils/utils.h"
#include "utils/list.h"

struct mcp_tool *mcp_tool_create(const char *name,
                            const char *description,
                            const struct mcp_property_list *properties,
                            mcp_tool_callback_t callback,
                            void *user_ctx,
                            bool user_only)
{
    struct mcp_tool *tool = (struct mcp_tool *)calloc(1, sizeof(struct mcp_tool));
    if (!tool) {
        return NULL;
    }
    tool->name = utils_strdup(name);
    tool->description = utils_strdup(description);
    tool->properties = mcp_property_list_clone(properties);
    tool->callback = callback;
    tool->user_ctx = user_ctx;
    tool->user_only = user_only;
    if (!tool->name || !tool->description) {
        mcp_tool_destroy(tool);
        return NULL;
    }
    return tool;
}

void mcp_tool_destroy(struct mcp_tool *tool)
{
    if (!tool) {
        return;
    }
    free(tool->name);
    free(tool->description);
    mcp_property_list_free(&tool->properties);
    free(tool);
}

char *mcp_tool_to_json(const struct mcp_tool *tool)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return NULL;
    }
    cJSON_AddStringToObject(root, "name", tool->name);
    cJSON_AddStringToObject(root, "description", tool->description);

    cJSON *input_schema = cJSON_CreateObject();
    cJSON_AddStringToObject(input_schema, "type", "object");
    cJSON_AddItemToObject(root, "inputSchema", input_schema);

    cJSON *properties = cJSON_CreateObject();
    cJSON_AddItemToObject(input_schema, "properties", properties);

    cJSON *required = cJSON_CreateArray();
    bool has_required = false;

    const struct list *l = PROPERTY_LIST_TO_LIST((struct mcp_property_list *)&tool->properties);
    size_t count = list_count(l);
    for (size_t i = 0; i < count; ++i) {
        const struct mcp_property *property = (const struct mcp_property *)list_get_const(l, i);
        if (!property->has_default) {
            cJSON_AddItemToArray(required, cJSON_CreateString(property->name));
            has_required = true;
        }
        if (!mcp_property_append_json(properties, property)) {
            cJSON_Delete(root);
            return NULL;
        }
    }

    if (has_required) {
        cJSON_AddItemToObject(input_schema, "required", required);
    } else {
        cJSON_Delete(required);
    }

    if (tool->user_only) {
        cJSON *annotations = cJSON_CreateObject();
        cJSON *audience = cJSON_CreateArray();
        cJSON_AddItemToArray(audience, cJSON_CreateString("user"));
        cJSON_AddItemToObject(annotations, "audience", audience);
        cJSON_AddItemToObject(root, "annotations", annotations);
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

char *mcp_tool_call(const struct mcp_tool *tool, const struct mcp_property_list *arguments)
{
    if (!tool || !tool->callback) {
        return NULL;
    }
    struct mcp_return_value value = tool->callback(arguments, tool->user_ctx);
    char *json = mcp_build_content_from_return(&value);
    mcp_return_value_free(&value);
    return json;
}

