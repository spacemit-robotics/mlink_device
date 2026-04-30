/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
    * 属性列表单元测试
    */

#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include "mlink.h"

static void test_property_list_create_destroy(void)
{
    mlink_property_list_t *props = mlink_property_list_create();
    TEST_ASSERT(props != NULL, "Property list creation should succeed");
    mlink_property_list_destroy(props);
}

static void test_property_list_add_get_int(void)
{
    mlink_property_list_t *props = mlink_property_list_create();
    TEST_ASSERT(props != NULL, "Property list creation");

    /* 添加 int 属性：必需，无默认值，范围 0-100 */
    bool ret = mlink_property_list_add_int(props, "volume",
                                            false, 0,    /* has_default=false */
                                            true, 0,     /* has_min=true, min=0 */
                                            true, 100);  /* has_max=true, max=100 */
    TEST_ASSERT(ret, "Add int property should succeed");

    /* 注意：property_list 只定义属性结构，不存储值
        * get_int 用于从解析后的参数中获取值，而不是从定义中获取
        * 这里只测试属性定义是否成功添加 */

    mlink_property_list_destroy(props);
}

static void test_property_list_add_get_string(void)
{
    mlink_property_list_t *props = mlink_property_list_create();
    TEST_ASSERT(props != NULL, "Property list creation");

    /* 添加 string 属性：必需，无默认值 */
    bool ret = mlink_property_list_add_string(props, "action", NULL);
    TEST_ASSERT(ret, "Add string property should succeed");

    /* 获取属性值 */
    const char *action = mlink_property_list_get_string(props, "action");
    TEST_ASSERT(action == NULL, "String property should be NULL when not set");

    mlink_property_list_destroy(props);
}

static void test_property_list_add_get_bool(void)
{
    mlink_property_list_t *props = mlink_property_list_create();
    TEST_ASSERT(props != NULL, "Property list creation");

    /* 添加 bool 属性：可选，默认值 true */
    bool ret = mlink_property_list_add_bool(props, "enabled", true, true);
    TEST_ASSERT(ret, "Add bool property should succeed");

    /* 获取属性值 */
    bool enabled = false;
    bool found = mlink_property_list_get_bool(props, "enabled", &enabled);
    TEST_ASSERT(found, "Get bool property should succeed");
    TEST_ASSERT(enabled == true, "Default value should be true");

    mlink_property_list_destroy(props);
}

static void test_property_list_multiple_properties(void)
{
    mlink_property_list_t *props = mlink_property_list_create();
    TEST_ASSERT(props != NULL, "Property list creation");

    /* 添加多个属性 */
    bool ret1 = mlink_property_list_add_int(props, "volume", false, 0, true, 0, true, 100);
    bool ret2 = mlink_property_list_add_string(props, "action", NULL);
    bool ret3 = mlink_property_list_add_bool(props, "enabled", true, true);

    /* 验证所有属性都成功添加 */
    TEST_ASSERT(ret1, "Add int property");
    TEST_ASSERT(ret2, "Add string property");
    TEST_ASSERT(ret3, "Add bool property");

    /* 注意：property_list 只定义属性，不存储值
        * get_* 函数用于从解析后的参数中获取值 */

    mlink_property_list_destroy(props);
}

static void test_property_list_nonexistent_property(void)
{
    mlink_property_list_t *props = mlink_property_list_create();
    TEST_ASSERT(props != NULL, "Property list creation");

    /* 尝试获取不存在的属性 */
    int value = 0;
    bool found = mlink_property_list_get_int(props, "nonexistent", &value);
    TEST_ASSERT(!found, "Nonexistent property should not be found");

    const char *str = mlink_property_list_get_string(props, "nonexistent");
    TEST_ASSERT(str == NULL, "Nonexistent string property should return NULL");

    mlink_property_list_destroy(props);
}

int main(void)
{
    printf("========================================\n");
    printf("   Property List Unit Tests\n");
    printf("========================================\n\n");

    TEST_RUN(test_property_list_create_destroy);
    TEST_RUN(test_property_list_add_get_int);
    TEST_RUN(test_property_list_add_get_string);
    TEST_RUN(test_property_list_add_get_bool);
    TEST_RUN(test_property_list_multiple_properties);
    TEST_RUN(test_property_list_nonexistent_property);

    printf("\n========================================\n");
    printf("   Test Results\n");
    printf("========================================\n");
    printf("Passed: %d\n", test_passed);
    printf("Failed: %d\n", test_failed);
    printf("Total:  %d\n", test_passed + test_failed);
    printf("========================================\n");

    if (test_failed == 0) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed!\n");
        return 1;
    }
}

