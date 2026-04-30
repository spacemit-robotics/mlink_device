/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LOG_H
#define LOG_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>
#define MLINK_LOG_TAG "MLINK"

#define MLINK_INFO(fmt, ...)  printf("[INFO] %s,%d " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define MLINK_ERROR(fmt, ...) printf("[ERROR] %s,%d " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define MLINK_DEBUG(fmt, ...) printf("[DEBUG] %s,%d " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define MLINK_WARNG(fmt, ...) printf("[WARNG] %s,%d " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define MLINK_DUMP(fmt, ...)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LOG_H */
