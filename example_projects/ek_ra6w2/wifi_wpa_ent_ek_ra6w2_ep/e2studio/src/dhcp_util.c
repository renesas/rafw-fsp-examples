/***********************************************************************************************************************
 * File Name    : dhcp_util.c
 * Description  : dhcp util api
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "lwip/opt.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/ip_addr.h"
#include "dhcp_util.h"
#include "net_dhcp_client.h"
#include <stdio.h>

/**
 * Get DHCP server IPv4 address associated with a netif.
 *
 * @param netif   Pointer to network interface that uses DHCPv4.
 * @param out     Output pointer to ip_addr_t that will receive the server IP.
 * @return 0 on success, negative on error:
 *         -1 invalid args, -2 DHCP not enabled/attached, -3 not BOUND yet.
 */
int dhcp_get_server_ip(const struct netif *netif, ip_addr_t *out) {
#if LWIP_DHCP
    if (!netif || !out) return -1;

    // In lwIP 2.x, DHCP client data is stored as "client data" on netif.
    struct dhcp *dhcp = (struct dhcp *)netif_get_client_data((struct netif *)netif,
                                   LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);
    if (!dhcp) return -2;

    // Ensure we are bound (i.e., have a lease).
    if (dhcp->state != DHCP_STATE_BOUND) {
        return -3; // Not bound yet
    }

    // dhcp->server_ip_addr holds the DHCP server's IPv4 address (ip_addr_t)
    *out = dhcp->server_ip_addr;
    return 0;
#else
    LWIP_UNUSED_ARG(netif);
    LWIP_UNUSED_ARG(out);
    return -2; // DHCP disabled in lwipopts.h
#endif
}
