/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MCP_SERVER_PROTOCOL_H
#define MCP_SERVER_PROTOCOL_H

#include "mcp_server.h"
#include <cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MCP protocol message handling. */
void mcp_server_parse_message(struct mcp_server *server, const char *message);

/* Connection establishment and capability announcement. */
void mcp_server_send_hello(struct mcp_server *server, const char *transport_name);

/* Send log / notification messages (MCP notifications/message). */
void mcp_server_send_notification_message(struct mcp_server *server,
    const char *level,
    const char *logger,
    const char *text);

#ifdef __cplusplus
}
#endif

#endif /* MCP_SERVER_PROTOCOL_H */

