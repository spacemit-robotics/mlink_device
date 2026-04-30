/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mcp/mcp_return_value.h"
#include <stdlib.h>
#include <stdio.h>
#include <cJSON.h>
#include "utils/utils.h"

void mcp_image_content_free(struct mcp_image_content *content)
{
    if (!content) {
        return;
    }
    free(content->mime_type);
    free(content->encoded_data);
    free(content);
}

struct mcp_return_value mcp_return_bool(bool value)
{
    struct mcp_return_value rv;
    rv.type = MCP_RETURN_TYPE_BOOL;
    rv.data.bool_value = value;
    return rv;
}

struct mcp_return_value mcp_return_int(int value)
{
    struct mcp_return_value rv;
    rv.type = MCP_RETURN_TYPE_INT;
    rv.data.int_value = value;
    return rv;
}

struct mcp_return_value mcp_return_string(const char *value)
{
    struct mcp_return_value rv;
    rv.type = MCP_RETURN_TYPE_STRING;
    rv.data.string_value = utils_strdup(value ? value : "");
    return rv;
}

struct mcp_return_value mcp_return_json(cJSON *json)
{
    struct mcp_return_value rv;
    rv.type = MCP_RETURN_TYPE_JSON;
    rv.data.json_value = json;
    return rv;
}

struct mcp_return_value mcp_return_image(struct mcp_image_content *image)
{
    struct mcp_return_value rv;
    rv.type = MCP_RETURN_TYPE_IMAGE;
    rv.data.image_value = image;
    return rv;
}

void mcp_return_value_free(struct mcp_return_value *value)
{
    if (!value) {
        return;
    }
    switch (value->type) {
        case MCP_RETURN_TYPE_STRING:
            free(value->data.string_value);
            break;
        case MCP_RETURN_TYPE_JSON:
            cJSON_Delete(value->data.json_value);
            break;
        case MCP_RETURN_TYPE_IMAGE:
            mcp_image_content_free(value->data.image_value);
            break;
        default:
            break;
    }
    value->type = MCP_RETURN_TYPE_STRING;
    value->data.string_value = NULL;
}

char *mcp_build_content_from_return(const struct mcp_return_value *value)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return NULL;
    }
    cJSON *content = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "content", content);
    cJSON_AddBoolToObject(root, "isError", false);

    cJSON *entry = cJSON_CreateObject();
    cJSON_AddItemToArray(content, entry);

    switch (value->type) {
        case MCP_RETURN_TYPE_BOOL:
            cJSON_AddStringToObject(entry, "type", "text");
            cJSON_AddStringToObject(entry, "text", value->data.bool_value ? "true" : "false");
            break;
        case MCP_RETURN_TYPE_INT: {
            char buffer[16];
            snprintf(buffer, sizeof(buffer), "%d", value->data.int_value);
            cJSON_AddStringToObject(entry, "type", "text");
            cJSON_AddStringToObject(entry, "text", buffer);
            break;
        }
        case MCP_RETURN_TYPE_STRING:
            cJSON_AddStringToObject(entry, "type", "text");
            cJSON_AddStringToObject(entry, "text", value->data.string_value ? value->data.string_value : "");
            break;
        case MCP_RETURN_TYPE_JSON: {
            cJSON_AddStringToObject(entry, "type", "text");
            char *json_str = cJSON_PrintUnformatted(value->data.json_value);
            cJSON_AddStringToObject(entry, "text", json_str ? json_str : "");
            free(json_str);
            break;
        }
        case MCP_RETURN_TYPE_IMAGE: {
            cJSON_AddStringToObject(entry, "type", "image");
            if (value->data.image_value) {
                cJSON_AddStringToObject(
                    entry,
                    "image",
                    value->data.image_value->encoded_data ? value->data.image_value->encoded_data : "");
            }
            break;
        }
        default:
            break;
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

