/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIX_H
#define UNIX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Unix domain socket transport initialization - register into transport layer. */
int transport_unix_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UNIX_H */


