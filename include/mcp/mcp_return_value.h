/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MCP_RETURN_VALUE_H
#define MCP_RETURN_VALUE_H

#include "mcp_server.h"
#include <stdbool.h>
#include <cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

void mcp_image_content_free(struct mcp_image_content *content);

struct mcp_return_value mcp_return_bool(bool value);
struct mcp_return_value mcp_return_int(int value);
struct mcp_return_value mcp_return_string(const char *value);
struct mcp_return_value mcp_return_json(cJSON *json);
struct mcp_return_value mcp_return_image(struct mcp_image_content *image);
void mcp_return_value_free(struct mcp_return_value *value);

char *mcp_build_content_from_return(const struct mcp_return_value *value);

#ifdef __cplusplus
}
#endif

#endif /* MCP_RETURN_VALUE_H */

