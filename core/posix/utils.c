/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils/utils.h"

#include <stdlib.h>
#include <string.h>

char *utils_strdup(const char *src)
{
    if (!src) {
        return NULL;
    }
    size_t len = strlen(src);
    char *dest = (char *)malloc(len + 1);
    if (!dest) {
        return NULL;
    }
    memcpy(dest, src, len + 1);
    return dest;
}

