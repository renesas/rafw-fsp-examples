/***********************************************************************************************************************
 * File Name    : dhcp_util.h
 * Description  : dhcp util header file.
 ***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#define INVALID_ARGS    -1
#define DHCP_NOT_ENABLE -2
#define DHCP_NOT_BOUND  -3
#define DHCP_FINE_TIMER_RETRY_CNT 500

int dhcp_get_server_ip(const struct netif *netif, ip_addr_t *out);
