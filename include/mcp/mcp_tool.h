/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MCP_TOOL_H
#define MCP_TOOL_H

#include "mcp_server.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mcp_tool *mcp_tool_create(const char *name,
    const char *description,
    const struct mcp_property_list *properties,
    mcp_tool_callback_t callback,
    void *user_ctx,
    bool user_only);
void mcp_tool_destroy(struct mcp_tool *tool);
char *mcp_tool_to_json(const struct mcp_tool *tool);
char *mcp_tool_call(const struct mcp_tool *tool, const struct mcp_property_list *arguments);

#ifdef __cplusplus
}
#endif

#endif /* MCP_TOOL_H */

