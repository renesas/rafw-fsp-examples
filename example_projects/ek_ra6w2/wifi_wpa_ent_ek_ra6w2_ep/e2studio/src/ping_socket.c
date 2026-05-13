/***********************************************************************************************************************
 * File Name    : ping_socket.c
 * Description  : ping socket api
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "lwip/inet_chksum.h"
#include "lwip/icmp.h"
#include "lwip/ip.h"
#include "lwip/ip4.h"
#include "lwip/sys.h"
#include "lwip/api.h"
#include "lwip/netdb.h"
#include <string.h>
#include <stdio.h>
#include "ping_socket.h"
#include "common_utils.h"

#ifndef PING_ID
#define PING_ID      0xAFAF
#endif

#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif

static u16_t ping_seq = 0;

static u16_t ping_checksum(void *data, size_t len) {
    return inet_chksum(data, len);
}

static int ping_send_echo(int s, const ip_addr_t *addr) {
    struct icmp_echo_hdr *iecho;
    size_t icmp_len = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;
    char buf[sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE];
    memset(buf, 0, sizeof(buf));

    iecho = (struct icmp_echo_hdr *)buf;
    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id     = lwip_htons(PING_ID);
    iecho->seqno  = lwip_htons(++ping_seq);

    // Fill payload with a simple pattern
    for (int i = 0; i < PING_DATA_SIZE; i++) {
        buf[sizeof(struct icmp_echo_hdr) + i] = (char)i;
    }

    // Calculate ICMP checksum
    iecho->chksum = ping_checksum(buf, icmp_len);

    // Destination address (ICMP has no port)
    struct sockaddr_in to = {0};
    to.sin_family = AF_INET;
#ifdef LWIP_IPV4
    to.sin_addr.s_addr = ip_2_ip4(addr)->addr;
#else
    to.sin_addr.s_addr = ((const struct in_addr *)addr)->s_addr;
#endif
    to.sin_port = 0;

    int sent = sendto(s, buf, icmp_len, 0,
                      (struct sockaddr *)&to, sizeof(to));
    return sent == (int)icmp_len ? 0 : -1;
}

static int ping_recv_echo(int s, ip_addr_t *from, u32_t timeout_ms, u32_t *rtt_ms) {
    // Buffer size allows IP header + ICMP header + payload
    char buf[128 + sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE];
    struct sockaddr_in src;
    socklen_t srclen = sizeof(src);

    // Set receive timeout
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    u32_t t0 = sys_now();
    int n = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&src, &srclen);
    u32_t t1 = sys_now();

    if (n < 0) {
        // Timeout or error
        return -1;
    }

    // Incoming buffer may include an IP header followed by ICMP
    if (n < (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr))) {
        return -1;
    }

    // Skip IPv4 header by using its IHL (header length)
    struct ip_hdr *iph = (struct ip_hdr *)buf;
    u16_t iphdr_len = (IPH_HL(iph) * 4);
    if (n < iphdr_len + (int)sizeof(struct icmp_echo_hdr)) {
        return -1;
    }

    // Parse ICMP section
    struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)(buf + iphdr_len);
    if ((ICMPH_TYPE(iecho) == ICMP_ER) && (iecho->id == lwip_htons(PING_ID))) {
        if (from) {
            // Convert source address to ip_addr_t
            ip4_addr_t a4;
            a4.addr = src.sin_addr.s_addr;
            ip_addr_set_ip4_u32(from, a4.addr);
        }
        if (rtt_ms) *rtt_ms = t1 - t0;
        return 0;
    }
    return -1;
}

int lwip_ping(const char *host, int count, int timeout_ms) {
    ip_addr_t target_addr;

    // Resolve hostname or dotted IPv4 string to ip_addr_t
    err_t err = netconn_gethostbyname(host, &target_addr);
    if (err != ERR_OK) {
        APP_PRINT("resolve failed: %s\n", host);
        return -1;
    }

    // Create RAW ICMP socket
    int s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
    if (s < 0) {
        APP_PRINT("socket open failed\n");
        return -1;
    }

    APP_PRINT("\nPING %s (%s): %d data bytes\n",
           host, ipaddr_ntoa(&target_addr), PING_DATA_SIZE);

    int success = 0;
    for (int i = 0; i < count; i++) {
        if (ping_send_echo(s, &target_addr) == 0) {
            u32_t rtt = 0;
            ip_addr_t from;
            if (ping_recv_echo(s, &from, timeout_ms, &rtt) == 0) {
                APP_PRINT("%d bytes from %s: icmp_seq=%d time=%ums\n",
                       PING_DATA_SIZE, ipaddr_ntoa(&from), ping_seq, (unsigned)rtt);
                success++;
            } else {
                APP_PRINT("Request timeout for icmp_seq %d\n", ping_seq);
            }
        } else {
            APP_PRINT("send failed (seq=%d)\n", ping_seq + 1);
        }
        // Interval between requests
        sys_msleep(1000);
    }

    lwip_close(s);
    APP_PRINT("\n--- %s ping statistics ---\n", host);
    APP_PRINT("%d packets transmitted, %d received, %d%% packet loss\n",
           count, success, (count - success) * 100 / (count > 0 ? count : 1));
    return (success > 0) ? 0 : -1;
}
