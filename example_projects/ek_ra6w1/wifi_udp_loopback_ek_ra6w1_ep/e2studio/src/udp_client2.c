/***********************************************************************************************************************
 * File Name    : udp_client2.c
 * Description  : Contains function definitions.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include <udp_loopback.h>
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/err.h"
#include "FreeRTOS.h"
#include "task.h"
#include "udp_loopback.h"

static int g_packet_lose = 0;

/*
 * UDP client task that sends messages to a server, waits for responses,
 * and tracks packet loss. Closes socket and deletes itself when done.
 */
static void udp_loopback_client_task2(void *pvParameters)
{
    (void)pvParameters;
    int udp_socket;
    struct sockaddr_in server_addr;
    char send_buffer[UDP_CLIENT_BUFFER_SIZE];
    char recv_buffer[UDP_CLIENT_BUFFER_SIZE];
    int recv_len;
    int sent_count = 0;
    socklen_t server_addr_len = sizeof(server_addr);
    int bytes;
    udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (udp_socket < 0)
    {
        APP_PRINT_INFO("Failed to create socket\n");
        vTaskDelete(NULL);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(UDP_SERVER_IP);
    server_addr.sin_port = htons(UDP_SERVER_PORT);

    while (sent_count < MAX_DATA_COUNT)
    {
        snprintf(send_buffer, UDP_CLIENT_BUFFER_SIZE, "Hi from CLIENT-2, streaming data %d", sent_count);
        if ((bytes = sendto(udp_socket, send_buffer, strlen(send_buffer), 0,
                            (struct sockaddr *)&server_addr, server_addr_len)) < 0)
        {
            APP_PRINT_INFO("Failed to send message\n");
        }
        else
        {
            recv_len = recvfrom(udp_socket, recv_buffer, UDP_CLIENT_BUFFER_SIZE - 1, 0,
                                (struct sockaddr *)&server_addr, &server_addr_len);

            if (recv_len > 0)
            {
                recv_buffer[recv_len] = '\0';
                APP_PRINT_INFO("Client 2: Received response: %s\n", recv_buffer);
            }
            else
            {
                APP_PRINT_INFO("Error: Timeout!!! Client 2-No response received\n");
                g_packet_lose++;
            }
        }

        sent_count++;
    }

    APP_PRINT_INFO("CLIENT2 packet lose: %d%%\n", (g_packet_lose / sent_count) * 100);
    close(udp_socket);
    vTaskDelete(NULL);
}

/*
 * Initializes UDP Client 2 by creating its task.
 * Logs an error if task creation fails.
 */
void udp_client2_init(void)
{
    int status;
    TaskHandle_t hClient2 = NULL;
    status = xTaskCreate(udp_loopback_client_task2, "UDPLoopbackClient2",
                         TASK_STACK_SIZE, NULL, TASK_PRIORITY, &hClient2);

    if (pdPASS != status)
    {
        APP_PRINT_INFO("UDPLoopbackClient2 failed!!!\n");
    }
}
