/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
    * 返回值单元测试
    */

#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include "mlink.h"

static void test_return_bool(void)
{
    struct mlink_return_value rv = mlink_return_bool(true);
    TEST_ASSERT(rv.type == MLINK_RETURN_TYPE_BOOL, "Return type should be BOOL");
    TEST_ASSERT(rv.data.bool_value == true, "Bool value should be true");

    rv = mlink_return_bool(false);
    TEST_ASSERT(rv.data.bool_value == false, "Bool value should be false");
}

static void test_return_int(void)
{
    struct mlink_return_value rv = mlink_return_int(42);
    TEST_ASSERT(rv.type == MLINK_RETURN_TYPE_INT, "Return type should be INT");
    TEST_ASSERT(rv.data.int_value == 42, "Int value should be 42");

    rv = mlink_return_int(-100);
    TEST_ASSERT(rv.data.int_value == -100, "Int value should be -100");
}

static void test_return_string(void)
{
    const char *test_str = "Hello, World!";
    struct mlink_return_value rv = mlink_return_string(test_str);
    TEST_ASSERT(rv.type == MLINK_RETURN_TYPE_STRING, "Return type should be STRING");
    TEST_ASSERT(rv.data.string_value != NULL, "String value should not be NULL");
    TEST_ASSERT(strcmp(rv.data.string_value, test_str) == 0, "String value should match");

    /* 测试 NULL 字符串 */
    rv = mlink_return_string(NULL);
    TEST_ASSERT(rv.type == MLINK_RETURN_TYPE_STRING, "Return type should be STRING");
    /* NULL 字符串的处理取决于实现 */
}

static void test_return_image(void)
{
    struct mlink_image_content *image = (struct mlink_image_content *)malloc(sizeof(struct mlink_image_content));
    TEST_ASSERT(image != NULL, "Image content allocation");

    image->mime_type = strdup("image/png");
    image->encoded_data = strdup("base64encodeddata");

    struct mlink_return_value rv = mlink_return_image(image);
    TEST_ASSERT(rv.type == MLINK_RETURN_TYPE_IMAGE, "Return type should be IMAGE");
    TEST_ASSERT(rv.data.image_value != NULL, "Image value should not be NULL");
    TEST_ASSERT(rv.data.image_value == image, "Image value should match");

    /* 清理 */
    mlink_image_content_free(image);
}

static void test_return_value_free(void)
{
    /* 测试字符串返回值的释放 */
    struct mlink_return_value rv = mlink_return_string("test string");
    mlink_return_value_free(&rv);
    /* 释放后不应该崩溃 */

    /* 测试图像返回值的释放 */
    struct mlink_image_content *image = (struct mlink_image_content *)malloc(sizeof(struct mlink_image_content));
    image->mime_type = strdup("image/png");
    image->encoded_data = strdup("data");
    rv = mlink_return_image(image);
    mlink_return_value_free(&rv);
    /* 释放后不应该崩溃 */
}

static void test_return_value_types(void)
{
    /* 测试所有返回类型 */
    struct mlink_return_value rv;

    rv = mlink_return_bool(true);
    TEST_ASSERT(rv.type == MLINK_RETURN_TYPE_BOOL, "Type BOOL");

    rv = mlink_return_int(123);
    TEST_ASSERT(rv.type == MLINK_RETURN_TYPE_INT, "Type INT");

    rv = mlink_return_string("test");
    TEST_ASSERT(rv.type == MLINK_RETURN_TYPE_STRING, "Type STRING");

    struct mlink_image_content *img = (struct mlink_image_content *)malloc(sizeof(struct mlink_image_content));
    img->mime_type = strdup("image/jpeg");
    img->encoded_data = strdup("data");
    rv = mlink_return_image(img);
    TEST_ASSERT(rv.type == MLINK_RETURN_TYPE_IMAGE, "Type IMAGE");
    mlink_image_content_free(img);
}

int main(void)
{
    printf("========================================\n");
    printf("   Return Value Unit Tests\n");
    printf("========================================\n\n");

    TEST_RUN(test_return_bool);
    TEST_RUN(test_return_int);
    TEST_RUN(test_return_string);
    TEST_RUN(test_return_image);
    TEST_RUN(test_return_value_free);
    TEST_RUN(test_return_value_types);

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

