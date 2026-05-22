/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
    * 测试框架公共头文件
    * 提供统一的测试宏和辅助函数
    */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 测试框架宏 ==================== */

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("[FAIL] %s:%d: %s\n", __FILE__, __LINE__, message); \
            test_failed++; \
            return; \
        } \
    } while (0)

#define TEST_RUN(test_func) \
    do { \
        printf("\n=== Running: %s ===\n", #test_func); \
        int prev_failed = test_failed; \
        test_func(); \
        if (test_failed == prev_failed) { \
            printf("[PASS] %s\n", #test_func); \
            test_passed++; \
        } \
    } while (0)

/* 外部变量声明 */
extern int test_passed;
extern int test_failed;

/* ==================== JSON 验证辅助函数 ==================== */

static inline bool json_has_string(cJSON *json, const char *key, const char *expected)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (!item || !cJSON_IsString(item)) {
        return false;
    }
    if (expected && strcmp(item->valuestring, expected) != 0) {
        return false;
    }
    return true;
}

static inline bool json_has_bool(cJSON *json, const char *key, bool expected)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (!item || !cJSON_IsBool(item)) {
        return false;
    }
    return (cJSON_IsTrue(item) != 0) == expected;
}

static inline bool json_has_number(cJSON *json, const char *key, int expected)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (!item || !cJSON_IsNumber(item)) {
        return false;
    }
    return item->valueint == expected;
}

static inline bool json_has_double(cJSON *json, const char *key, double expected, double epsilon)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (!item || !cJSON_IsNumber(item)) {
        return false;
    }
    double diff = item->valuedouble - expected;
    if (diff < 0) diff = -diff;
    return diff < epsilon;
}

#ifdef __cplusplus
}
#endif

#endif

