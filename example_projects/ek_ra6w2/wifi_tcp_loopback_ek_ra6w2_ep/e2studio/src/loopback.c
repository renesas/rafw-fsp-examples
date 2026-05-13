/***********************************************************************************************************************
 * File Name    : loopback.c
 * Description  : Contains data structures and functions used in loopback.c.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lwip/netdb.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"
#include "lwip/sockets.h"
#include "common_utils.h"
#include "loopback.h"

#define SERVER_PORT 5000
#define BUFFER_SIZE 256
#define MAX_CLIENTS 5
#define MAX_DATA_LEN 1
/*******************************************************************************************************************//**
 * @addtogroup rm_lwip_ep
 * @{
 **********************************************************************************************************************/

/*
 * Private Global Variables
 */
static int num_client = 0;
static int client_num = 0;
extern TaskHandle_t g_taskHandle;

/*
 * This task creates a TCP server socket, binds it to a port, and listens
 * for incoming client connections. For each accepted client connection,
 * a new client handler task is created to process communication.
 *
 * Errors such as socket creation, bind, or accept failures are logged,
 * and the server continues running. However, if memory allocation for a
 * client socket fails, the server task will terminate.
 */
void tcp_server_task(void *pvParameters)
{
    (void)pvParameters;
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    APP_PRINT_INFO("start TCP Server\n");
    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock < 0)
    {
        APP_PRINT_INFO("Failed to create socket\n");
        vTaskDelete(NULL);
    }

    /*
     * Configure server address
     */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    /*
     * Bind socket
     */
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        APP_PRINT_INFO("Failed to bind socket\n");
        close(server_sock);
        vTaskDelete(NULL);
    }

    /*
     * Listen for connections
     */
    listen(server_sock, MAX_CLIENTS);
    APP_PRINT_INFO("Server listening on port %d\n", SERVER_PORT);
    xTaskNotifyGive(g_taskHandle);
    while (1)
    {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0)
        {
            APP_PRINT_INFO("Failed to accept connection\n");
            continue;
        }

        /*
         * Create a task for each client
         */
        int *p_client_sock = malloc(sizeof(int));
        if (!p_client_sock)
        {
            APP_PRINT_INFO("system out of space\n");
            break;
        }

        *p_client_sock = client_sock;
        xTaskCreate(client_handler_task, "Client Handler Task", 4096, p_client_sock, 5, NULL);
    }

    vTaskDelete(NULL);
}

/*
 * This task handles communication with a single connected client.
 * It receives data from the client, logs it, and echoes the data back.
 *
 * Each client is assigned a unique ID for identification in logs.
 * If the client disconnects or an error occurs, the socket is closed,
 * and the task is terminated to free resources.
 */
void client_handler_task(void *pvParameters)
{
    int client_sock = *((int *)pvParameters);
    free(pvParameters);
    int client_id = ++num_client;
    char buffer[BUFFER_SIZE];
    int len;

    APP_PRINT_INFO("Client%d connected\n", client_id);
    while ((len = recv(client_sock, buffer, BUFFER_SIZE - 1, 0)) > 0)
    {
        if (len == 0)
        {
            APP_PRINT_INFO("Client%d disconnected\n", client_id);
            break;
        }
        else if (len < 0)
        {
            APP_PRINT_INFO("Error receiving from Client%d\n", client_id);
        }
        else
        {
            buffer[len] = '\0';
            APP_PRINT_INFO("Received from client%d: %s\n",client_id, buffer);
            send(client_sock, buffer, len, 0); // Echo back
        }
    }

    close(client_sock);
    APP_PRINT_INFO("Client disconnected\n");
    vTaskDelete(NULL);
}

/*
 * This task creates a TCP client socket and connects it to the server.
 * It sends a message repeatedly to the server, waits for the echo response,
 * and tracks any failed receptions as packet loss.
 *
 * Each client is assigned a unique ID for logging. After completing the
 * transmission loop, the packet loss percentage is reported, the socket
 * is closed, and the task is terminated.
 */
void tcp_client_task(void *pvParameters)
{
    (void)pvParameters;
    int client_sock;
    struct sockaddr_in server_addr;
    int clientid = ++client_num;
    char send_buffer[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE];
    int send_count = 0;
    int packet_lose = 0;

    APP_PRINT_INFO("start TCP Client%d\n", clientid);
    snprintf(send_buffer, BUFFER_SIZE, "Hello from Client%d!", clientid);
    client_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (client_sock < 0)
    {
        APP_PRINT_INFO("Failed to create client socket\n");
        vTaskDelete(NULL);
    }

    /*
     * Configure server address
     */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Localhost
    server_addr.sin_port = htons(SERVER_PORT);

    /*
     *  Connect to server
     */
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        APP_PRINT_INFO("Client%d Failed to connect to server\n", clientid);
        close(client_sock);
        vTaskDelete(NULL);
    }

    while (MAX_DATA_LEN > send_count)
    {
        send(client_sock, send_buffer, strlen(send_buffer), 0);
        send_count++;
        int len = recv(client_sock, recv_buffer, BUFFER_SIZE - 1, 0);
        if (len > 0)
        {
            recv_buffer[len] = '\0';
            APP_PRINT_INFO("Client%d Received echo: %s\n",clientid, recv_buffer);
        }
        else
        {
            APP_PRINT_INFO("received fail\n");
            packet_lose++;
        }
    }

    APP_PRINT_INFO("client%d packet lose: %d%%\n",clientid, (packet_lose / send_count) * 100);
    vTaskDelay(10);
    close(client_sock);
    vTaskDelete(NULL);
}
