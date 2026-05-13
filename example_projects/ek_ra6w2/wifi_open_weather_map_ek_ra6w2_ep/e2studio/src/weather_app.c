/***********************************************************************************************************************
 * File Name    : weather_app.c
 * Description  : Contains data structures and functions used in weather_app.c.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/
#include "common_data.h"
#include <string.h>
#include "https.h"
#include "common_utils.h"
#include "weather_app.h"
#include "hal_data.h"

/* ===================== MACROS ===================== */

#define API_KEY    "377d56938b6ee391b30f3d3eb5eed1a4"

#ifdef HTTPS
#define PORT       (443)
#else
#define PORT       (80)
#endif

#ifdef HTTPS
int ca_cert_len = sizeof(ROOT_CA);
#endif

/* ===================== GLOBAL VARIABLES ===================== */

int server_transfer_error    = 0;
int server_transfer_complete = 0;
int client_transfer_complete = 0;
int client_transfer_error    = 0;

char current_loc[] = "Location";

extern TaskHandle_t g_app_main_task_handle;

/* ===================== EP INFO ===================== */

void print_ep_info_banner(void)
{
    fsp_pack_version_t version;

    R_FSP_VersionGet(&version);

    APP_PRINT(BANNER_1);
    APP_PRINT(BANNER_2);
    APP_PRINT(BANNER_3, EP_VERSION);
    APP_PRINT(BANNER_4,
              version.version_id_b.major,
              version.version_id_b.minor,
              version.version_id_b.patch);
    APP_PRINT(BANNER_5);
    APP_PRINT(BANNER_6);
    APP_PRINT(EP_INFO);
}

/* ===================== HTTPS CALLBACK ===================== */

void g_https0_callback(https_callback_args_t *p_args)
{
    switch (p_args->event)
    {
        case HTTPS_EVENT_SERVER_RECVED:
        {
            err_t err = *(int8_t *) p_args->p_param;

            if ((err != ERR_OK) || (p_args->len == 0))
            {
                server_transfer_error = 1;
            }
            else
            {
                server_transfer_complete = 1;
            }

            if (g_app_main_task_handle != NULL)
            {
                xTaskNotify(g_app_main_task_handle,
                            HTTPS_EVENT_SERVER_RECVED,
                            eSetValueWithOverwrite);
            }
            break;
        }

        case HTTPS_EVENT_SERVER_ERR_RESULT:
        {
            err_t err = *(int8_t *) p_args->p_param;

            if (err != ERR_OK)
            {
                server_transfer_error = 1;
            }

            if (g_app_main_task_handle != NULL)
            {
                xTaskNotify(g_app_main_task_handle,
                            HTTPS_EVENT_SERVER_ERR_RESULT,
                            eSetValueWithOverwrite);
            }
            break;
        }

        case HTTPS_EVENT_CLIENT_RESULT:
        {
            err_t err = *(int8_t *) p_args->p_param;

            if (err == ERR_OK)
            {
                client_transfer_complete = 1;
            }
            else
            {
                client_transfer_error = 1;
            }

            if (g_app_main_task_handle != NULL)
            {
                xTaskNotify(g_app_main_task_handle,
                            HTTPS_EVENT_CLIENT_RESULT,
                            eSetValueWithOverwrite);
            }
            break;
        }

        case HTTPS_EVENT_CLIENT_RECVED:
        {
            if (g_app_main_task_handle != NULL)
            {
                xTaskNotify(g_app_main_task_handle,
                            HTTPS_EVENT_CLIENT_RECVED,
                            eSetValueWithOverwrite);
            }

            /* Print received JSON weather response */
            print_wheather_info_json((void *) p_args);

            break;
        }

        case HTTPS_EVENT_CLIENT_RECVED_DECODED:
        {
            if (g_app_main_task_handle != NULL)
            {
                xTaskNotify(g_app_main_task_handle,
                            HTTPS_EVENT_CLIENT_RECVED_DECODED,
                            eSetValueWithOverwrite);
            }
            break;
        }

        case HTTPS_EVENT_CLIENT_GET_DONE:
        {
            if (g_app_main_task_handle != NULL)
            {
                xTaskNotify(g_app_main_task_handle,
                            HTTPS_EVENT_CLIENT_GET_DONE,
                            eSetValueWithOverwrite);
            }
            break;
        }

        default:
            break;
    }
}

/* ===================== CERT CONFIG ===================== */

fsp_err_t set_cert(httpc_secure_connection_t *https_conf,
                   u8 *cert,
                   size_t cert_len)
{
    https_conf->ca = (u8 *) malloc(cert_len);

    if (https_conf->ca == NULL)
    {
        return FSP_ERR_OUT_OF_MEMORY;
    }

    memcpy(https_conf->ca, cert, cert_len);
    https_conf->ca_len = cert_len;

    return FSP_SUCCESS;
}

/* ===================== REQUEST PREP ===================== */

void prepare_client_request_owm(https_client_opcode_t op_code,
                                http_client_request_t *req,
                                char *curr_location)
{
    req->op_code = op_code;
    req->port    = PORT;

#ifdef HTTPS
    req->insecure = pdTRUE;
    set_cert(&(req->https_conf), ROOT_CA, ca_cert_len);
#else
    req->insecure = pdFALSE;
#endif

    strncpy(req->hostname,
            "api.openweathermap.org",
            HTTPC_MAX_HOSTNAME_LEN);

    snprintf(req->path,
             HTTPC_MAX_PATH_LEN,
             "https://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s",
             curr_location,
             API_KEY);
}

/* ===================== WEATHER MONITOR ===================== */

fsp_err_t start_weather_monitor(void)
{
    static bool https_opened = false;
    http_client_request_t request;
    fsp_err_t err;

    if (!https_opened)
    {
        err = RM_HTTPS_W_Open((https_ctrl_t *) &g_https_w0_ctrl,
                              &g_https_w0_cfg);

        if (err != FSP_SUCCESS)
        {
            APP_PRINT("[ERROR] HTTPS open failed\n");
            return err;
        }

        https_opened = true;
    }


    prepare_client_request_owm(HTTP_CLIENT_OPCODE_GET,
                               &request,
                               current_loc);

    err = RM_HTTPS_W_ClientSendRequest(
                (https_ctrl_t *) &g_https_w0_ctrl,
                &request);

    if (err != FSP_SUCCESS)
    {
        APP_PRINT("[ERROR] HTTPS send failed\n");
        return err;
    }

    return err;
}

void stop_weather_monitor(void)
{
    RM_HTTPS_W_Close(&g_https_w0_ctrl);
}
