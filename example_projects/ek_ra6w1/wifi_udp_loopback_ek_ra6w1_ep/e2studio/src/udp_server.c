/***********************************************************************************************************************
 * File Name    : udp_server.c
 * Description  : Contains data structure and echo udp server function definitions.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include <udp_loopback.h>
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "stdio.h"
#include "udp_loopback.h"

#define MAX_CLIENT_CONN 2
#define SERVER_RUNNING 1
#define SERVER_STOP 0

typedef struct
{
    char message[UDP_SERVER_BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
} udp_message_t;

static int udp_socket = -1;
static int server_status = SERVER_RUNNING;
static TaskHandle_t g_svr_taskHandle;
static QueueHandle_t udp_queue;
extern TaskHandle_t g_taskHandle;

/*
 * UDP Receiver Task
 *
 * Listens for incoming UDP messages from clients. Received messages
 * are enqueued for the sender task, and a simple "- OK" acknowledgment
 * is appended. Handles socket creation, binding, and basic error logging.
 */
static void udp_receiver_task(void *pvParameters)
{
    (void)pvParameters;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char recv_buffer[UDP_SERVER_BUFFER_SIZE];
    int recv_len;
    udp_message_t message;
    int count = 0;
    g_svr_taskHandle = xTaskGetCurrentTaskHandle();

    /* Create socket if it is not created yet */
    if (udp_socket == -1)
    {
        udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (udp_socket < 0)
        {
            APP_PRINT_INFO("Failed to create socket\n");
            vTaskDelete(NULL);
        }
    }

    /* Bind the socket */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(UDP_SERVER_PORT);

    if (bind(udp_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        APP_PRINT_INFO("Socket bind failed\n");
        close(udp_socket);
        udp_socket = -1;  // Reset to indicate socket is closed
        vTaskDelete(NULL);
    }

    APP_PRINT_INFO("UDP server listening on port %d\n", UDP_SERVER_PORT);
    xTaskNotifyGive(g_taskHandle);
    while (count <= MAX_CLIENT_CONN)
    {
        recv_len = recvfrom(udp_socket, recv_buffer, UDP_SERVER_BUFFER_SIZE - 1, 0,
                            (struct sockaddr *)&client_addr, &client_addr_len);

        if (recv_len > 0)
        {
            recv_buffer[recv_len] = '\0';
            APP_PRINT_INFO("SERVER-received_data:%s\n", recv_buffer);
            memset(&message, 0, sizeof(message));
            strncpy(message.message, recv_buffer, recv_len);
            strcat(message.message, "- OK");
            recv_len += strlen("- OK");
            message.client_addr = client_addr;
            message.client_addr_len = client_addr_len;

            /* Send the received message to the sender task through the queue */
            if (pdPASS != xQueueSend(udp_queue, &message, 0))
            {
                APP_PRINT_INFO("Failed to send message to sender task\n");
            }
        }
        else
        {
            APP_PRINT_INFO("Error receiving data\n");
        }

        count++;
    }

    server_status = SERVER_STOP;

    /* wait for remaining data to send */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    close(udp_socket);
    vTaskDelete(NULL);
}

/* Sender Task: Handles sending data to clients  */
static void udp_echo_task(void *pvParameters)
{
    (void)pvParameters;
    udp_message_t message;

    while (pdPASS == xQueueReceive(udp_queue, &message, portMAX_DELAY))
    {
        /* Send the message back to the client  */
        if (udp_socket >= 0)
        {
            if (sendto(udp_socket, message.message, strlen(message.message), 0,
                       (struct sockaddr *)&message.client_addr, message.client_addr_len) < 0)
            {
                APP_PRINT_INFO("Error sending response\n");
            }
        }
    }

    xTaskNotifyGive(g_svr_taskHandle);
    vTaskDelete(NULL);
}

/*
 * Creates the UDP message queue and starts the receiver and echo tasks.
 * Returns 0 on success, -1 on failure to create the queue or tasks.
 */
int udp_server_init(void)
{
    int status;
    TaskHandle_t hUdprecv1 = NULL;
    TaskHandle_t hUdpEcho1 = NULL;
    udp_queue = xQueueCreate(QUEUE_SIZE, sizeof(udp_message_t));

    if (udp_queue == NULL)
    {
        APP_PRINT_INFO("Failed to create queue\n");

        return -1;
    }

    /* Create the UDP receiver and echo tasks */
    status = xTaskCreate(udp_receiver_task, "UDPReceiver", TASK_STACK_SIZE, NULL, SVR_RCV_PRIORITY, &hUdprecv1);

    if (pdPASS != status)
    {
        APP_PRINT_INFO("UDPeceiver Task failed to start!!!\n");

        return -1;
    }

    status = xTaskCreate(udp_echo_task, "UDPEcho", TASK_STACK_SIZE, NULL, SVR_SEND_PRIORITY, &hUdpEcho1);

    if (pdPASS != status)
    {
        APP_PRINT_INFO("UDP Echo Task failed to start!!!\n");
        vTaskDelete(hUdprecv1);

        return -1;
    }

    return 0;
}
