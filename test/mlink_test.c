/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
    * MLINK MCP Server Demo
    *
    * This is a complete usage example of MLINK MCP Server, demonstrating how to:
    * 1. Initialize the server and configure the transport
    * 2. Create and register tools
    */

#include "mlink.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global server instance. */
static mlink_server_t *g_server = NULL;

/* ==================== Callback functions ==================== */

/**
    * Send callback function - called when the server needs to send data.
    */
static void example_send_callback(const char *payload, void *ctx)
{
    (void)ctx;
    printf("[MCP SEND] %s\n", payload);
}

/**
    * Tool callback: set speaker volume.
    */
static struct mlink_return_value tool_set_volume_cb(const mlink_property_list_t *props, void *ctx)
{
    (void)ctx;
    int volume = 0;

    /* Retrieve required parameter: volume. */
    if (!mlink_property_list_get_int(props, "volume", &volume)) {
        return mlink_return_string("missing volume parameter");
    }

    printf("[TOOL] set_volume -> volume=%d\n", volume);

    /* Real volume control logic can be added here. */
    /* For example: audio_set_volume(volume); */

    return mlink_return_bool(true);
}

/**
    * Tool callback: movement action.
    */
static struct mlink_return_value tool_move_action_cb(const mlink_property_list_t *props, void *ctx)
{
    (void)ctx;
    const char *action = mlink_property_list_get_string(props, "action");
    int speed = 0;
    int duration = 0;

    /* Retrieve parameters (some have default values and may be absent). */
    mlink_property_list_get_int(props, "speed", &speed);
    mlink_property_list_get_int(props, "duration", &duration);

    printf("[TOOL] move_action -> action=%s speed=%d duration=%d\n",
            action ? action : "(null)", speed, duration);

    /* Real movement control logic can be added here. */
    /* For example: chassis_move(action, speed, duration); */

    return mlink_return_bool(true);
}

/* ==================== Tool registration ==================== */

/**
    * Register all tools to the server.
    */
static int register_tools(void)
{
    if (!g_server) {
        printf("Error: Server not initialized\n");
        return -1;
    }

    /* Tool 1: set volume. */
    mlink_property_list_t *volume_props = mlink_property_list_create();
    if (!volume_props) {
        printf("Error: Failed to create volume properties\n");
        return -1;
    }

    /* Add parameter volume: required, no default, range 0–100. */
    mlink_property_list_add_int(volume_props, "volume",
                                false, 0,    /* has_default=false, default_value=0 */
                                true, 0,     /* has_min=true, min_value=0 */
                                true, 100);  /* has_max=true, max_value=100 */

    mlink_tool_t *volume_tool = mlink_tool_create(
        "audio_speaker.set_volume",
        "Set the volume of the speaker output (0-100)",
        volume_props,
        tool_set_volume_cb,
        NULL,
        false);  /* user_only = false */

    if (!volume_tool) {
        printf("Error: Failed to create volume tool\n");
        mlink_property_list_destroy(volume_props);
        return -1;
    }

    if (!mlink_server_add_tool(g_server, volume_tool)) {
        printf("Error: Failed to add volume tool\n");
        mlink_tool_destroy(volume_tool);
        mlink_property_list_destroy(volume_props);
        return -1;
    }

    mlink_property_list_destroy(volume_props);
    printf(" Registered tool: self.audio_speaker.set_volume\n");

    /* Tool 2: movement action. */
    mlink_property_list_t *move_props = mlink_property_list_create();
    if (!move_props) {
        printf("Error: Failed to create move properties\n");
        return -1;
    }

    /* Add parameter action: required, no default. */
    mlink_property_list_add_string(move_props, "action", NULL);

    /* Add parameter speed: optional, default 50, range 0–100. */
    mlink_property_list_add_int(move_props, "speed",
                                true, 50,    /* has_default=true, default_value=50 */
                                true, 0,      /* has_min=true, min_value=0 */
                                true, 100);   /* has_max=true, max_value=100 */

    /* Add parameter duration: optional, default 1000, no min/max limit. */
    mlink_property_list_add_int(move_props, "duration",
                                true, 1000,   /* has_default=true, default_value=1000 */
                                false, 0,     /* has_min=false */
                                false, 0);   /* has_max=false */

    mlink_tool_t *move_tool = mlink_tool_create(
        "chassis.move_action",
        "Control robot chassis movement (forward/backward/left/right)",
        move_props,
        tool_move_action_cb,
        NULL,
        false);

    if (!move_tool) {
        printf("Error: Failed to create move tool\n");
        mlink_property_list_destroy(move_props);
        return -1;
    }

    if (!mlink_server_add_tool(g_server, move_tool)) {
        printf("Error: Failed to add move tool\n");
        mlink_tool_destroy(move_tool);
        mlink_property_list_destroy(move_props);
        return -1;
    }

    mlink_property_list_destroy(move_props);
    printf(" Registered tool: self.chassis.move_action\n");

    return 0;
}

/* ==================== Main program ==================== */

static void print_usage(const char *prog)
{
    printf("Usage: %s [tcp|unix] [server_name]\n", prog);
    printf("  transport  : tcp (default) or unix\n");
    printf("  server_name: optional, default \"demo_node.self.mlink_0\"\n");
}

int main(int argc, char *argv[])
{
    const char *transport_str = "tcp";
    const char *server_name = "demo_node.self.mlink_0";
    enum transport_type type = TRANSPORT_TYPE_TCP;

    if (argc > 1) {
        transport_str = argv[1];
    }
    if (argc > 2 && argv[2] && argv[2][0] != '\0') {
        server_name = argv[2];
    }

    if (strcmp(transport_str, "tcp") == 0) {
        type = TRANSPORT_TYPE_TCP;
    } else if (strcmp(transport_str, "unix") == 0) {
        type = TRANSPORT_TYPE_UNIX;
    } else {
        printf("Unknown transport: %s\n", transport_str);
        print_usage(argv[0]);
        return 1;
    }

    printf("========================================\n");
    printf("   MLINK MCP Server Demo\n");
    printf("========================================\n\n");

    /* 1. Initialize server. */
    printf("1. Initializing server with %s transport, name=\"%s\"...\n",
            transport_str, server_name);
    g_server = mlink_server_init(type, server_name);
    if (!g_server) {
        printf(" Failed to initialize server\n");
        return 1;
    }
    printf(" Server initialized successfully\n\n");

    /* 2. Set send callback. */
    printf("2. Setting send callback...\n");
    mlink_server_set_send_callback(g_server, example_send_callback, NULL);
    printf(" Send callback set\n\n");

    /* 3. Register tools. */
    printf("3. Registering tools...\n");
    if (register_tools() != 0) {
        printf(" Failed to register tools\n");
        return 1;
    }
    printf(" All tools registered\n\n");

    /* 4. Done. */
    printf("========================================\n");
    printf("   Server ready!\n");
    printf("========================================\n");

    /* Enter the mlink server run loop (blocking this thread). */
    mlink_server_run(g_server);

    return 0;
}
