/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MLINK_H
#define MLINK_H

/*
    * MLINK public API header
    *
    * Note: Several struct layouts here must remain consistent with the internal
    * MCP structures so that core/mlink.c can safely perform type casting.
    */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration only; do not expose the full cJSON API. */
typedef struct cJSON cJSON;

/* ==================== Transport types ==================== */

enum transport_type {
    TRANSPORT_TYPE_UNIX = 0,
    TRANSPORT_TYPE_TCP,
    TRANSPORT_TYPE_MQTT,
    TRANSPORT_TYPE_HTTP,
    TRANSPORT_TYPE_WS,
    TRANSPORT_TYPE_UART,
    TRANSPORT_TYPE_SPI
};

/* ==================== Return values and image content ==================== */

/* Must be binary compatible with struct mcp_image_content. */
struct mlink_image_content {
    char *mime_type;
    char *encoded_data;
};

/* Must be binary compatible with enum mcp_return_type. */
enum mlink_return_type {
    MLINK_RETURN_TYPE_BOOL   = 0,
    MLINK_RETURN_TYPE_INT    = 1,
    MLINK_RETURN_TYPE_STRING = 2,
    MLINK_RETURN_TYPE_JSON   = 3,
    MLINK_RETURN_TYPE_IMAGE  = 4
};

/* Must be binary compatible with struct mcp_return_value. */
struct mlink_return_value {
    enum mlink_return_type type;
    union {
        bool bool_value;
        int int_value;
        char *string_value;
        cJSON *json_value;
        struct mlink_image_content *image_value;
    } data;
};

/* ==================== Opaque pointer types ==================== */

/* Binary compatible with mcp_* types (cast in core/mlink.c). */
struct mcp_server;
struct mcp_tool;
struct mcp_property_list;

typedef struct mcp_server mlink_server_t;
typedef struct mcp_tool mlink_tool_t;
typedef struct mcp_property_list mlink_property_list_t;

/* ==================== Callback types ==================== */

typedef void (*mlink_send_callback_t)(const char *payload, void *user_ctx);
typedef struct mlink_return_value (*mlink_tool_callback_t)(const mlink_property_list_t *properties,
                                                            void *user_ctx);

/* ==================== Server management ==================== */

mlink_server_t *mlink_server_init(enum transport_type type, const char *server_name);
void mlink_server_set_send_callback(mlink_server_t *server,
                                    mlink_send_callback_t callback,
                                    void *ctx);

void mlink_server_parse_message_str(mlink_server_t *server, const char *message);
void mlink_server_destroy(mlink_server_t *server);

void mlink_server_run(mlink_server_t *server);

/* ==================== Notifications / logging ==================== */

void mlink_notify_message(mlink_server_t *server,
                            const char *level,
                            const char *logger,
                            const char *text);

/* ==================== Tool management ==================== */

mlink_tool_t *mlink_tool_create(const char *name,
                                const char *description,
                                const mlink_property_list_t *properties,
                                mlink_tool_callback_t callback,
                                void *user_ctx,
                                bool user_only);

void mlink_tool_destroy(mlink_tool_t *tool);

bool mlink_server_add_tool(mlink_server_t *server, mlink_tool_t *tool);

/* ==================== Property list management ==================== */

mlink_property_list_t *mlink_property_list_create(void);
void mlink_property_list_destroy(mlink_property_list_t *list);

bool mlink_property_list_add_bool(mlink_property_list_t *list,
                                    const char *name,
                                    bool has_default,
                                    bool default_value);

bool mlink_property_list_add_int(mlink_property_list_t *list,
                                    const char *name,
                                    bool has_default,
                                    int default_value,
                                    bool has_min,
                                    int min_value,
                                    bool has_max,
                                    int max_value);

bool mlink_property_list_add_string(mlink_property_list_t *list,
                                    const char *name,
                                    const char *default_value);

bool mlink_property_list_get_bool(const mlink_property_list_t *list,
                                    const char *name,
                                    bool *out_value);

bool mlink_property_list_get_int(const mlink_property_list_t *list,
                                    const char *name,
                                    int *out_value);

const char *mlink_property_list_get_string(const mlink_property_list_t *list,
                                            const char *name);

/* ==================== Return value creation and destruction ==================== */

struct mlink_return_value mlink_return_bool(bool value);
struct mlink_return_value mlink_return_int(int value);
struct mlink_return_value mlink_return_string(const char *value);
struct mlink_return_value mlink_return_json(cJSON *json);
struct mlink_return_value mlink_return_image(struct mlink_image_content *image);

void mlink_return_value_free(struct mlink_return_value *value);
void mlink_image_content_free(struct mlink_image_content *content);

#ifdef __cplusplus
}
#endif

#endif /* MLINK_H */


