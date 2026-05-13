/***********************************************************************************************************************
 * File Name    : udp_client1.c
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
static void udp_loopback_client_task1(void *pvParameters)
{
    (void)pvParameters;
    int udp_socket;
    struct sockaddr_in server_addr;
    char send_buffer[UDP_CLIENT_BUFFER_SIZE];
    char recv_buffer[UDP_CLIENT_BUFFER_SIZE];
    int recv_len;
    int sent_count = 0;
    socklen_t server_addr_len = sizeof(server_addr);
    struct timeval timeout = {0, CLIENT_TIMEOUT_MS * 1000}; // 5 seconds
    int bytes_sent;
    udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (udp_socket < 0)
    {
        APP_PRINT_INFO("Failed to create socket\n");
        vTaskDelete(NULL);
    }

    /*
     * Set a timeout for receiving data
     */
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(UDP_SERVER_IP);
    server_addr.sin_port = htons(UDP_SERVER_PORT);

    while (sent_count < MAX_DATA_COUNT)
    {
        snprintf(send_buffer, UDP_CLIENT_BUFFER_SIZE, "Hi from CLIENT-1, streaming data %d", sent_count);

        /*
         * Send data to the server
         */
        bytes_sent = sendto(udp_socket, send_buffer, strlen(send_buffer), 0,
                            (struct sockaddr *)&server_addr, server_addr_len);

        if (bytes_sent < 0)
        {
            APP_PRINT_INFO("Client 1: Failed to send message\n");
        }
        else
        {
            /*
             * Wait for a response from the server
             */
            recv_len = recvfrom(udp_socket, recv_buffer, UDP_CLIENT_BUFFER_SIZE - 1, 0,
                                (struct sockaddr *)&server_addr, &server_addr_len);

            if (recv_len > 0)
            {
                recv_buffer[recv_len] = '\0';
                APP_PRINT_INFO("Client 1: Received response: %s\n", recv_buffer);
            }
            else
            {
                APP_PRINT_INFO("Error: Timeout!!! Client 1-No response received\n");
                g_packet_lose++;
            }
        }

        sent_count++;
    }

    /*
     *  Cleanup: Close the socket and delete the task
     */
    APP_PRINT_INFO("CLIENT1 packet lose: %d%%\n", (g_packet_lose / sent_count) * 100);
    close(udp_socket);
    vTaskDelete(NULL);
}

/*
 * Initializes UDP Client 1 by creating its task.
 * Logs an error if task creation fails.
 */
void udp_client1_init(void)
{
    int status;
    TaskHandle_t hClient1 = NULL;

    /*
     * Create the client task
     */
    status = xTaskCreate(udp_loopback_client_task1, "UDPLoopbackClient1",
                         TASK_STACK_SIZE, NULL, TASK_PRIORITY, &hClient1);

    if (pdPASS != status)
    {
        APP_PRINT_INFO("UDP Loopback Client 1: Task creation failed\n");
    }
}
