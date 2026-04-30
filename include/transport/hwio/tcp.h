/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TCP_H
#define TCP_H

#ifdef __cplusplus
extern "C" {
#endif

/* TCP transport initialization - register TCP transport type into transport layer. */
int transport_tcp_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TCP_H */

