/***********************************************************************************************************************
 * File Name    : ota.c
 * Description  : HTTPS OTA Download using Renesas OTA Framework
 **********************************************************************************************************************/
#include "FreeRTOS.h"
#include "custom_config_sdk.h"
#include "common_data.h"
#include "common_utils.h"
#include "ota.h"
#include "sdk_defs.h"
#include "common_def.h"
#include "ota_update.h"
#include "ota_update_common.h"
#include "timers.h"
#include "rm_vee_flash_w_rrq_nvram.h"
#include <string.h>
#include "hal_data.h"
#include "rm_wifi_helper.h"
#include "bsp_api.h"
#include "cmsis_gcc.h"

/***********************************************************************************************************************
 * EXTERNALS
 **********************************************************************************************************************/
extern TaskHandle_t g_app_main_task_handle;
extern uint32_t g_header_done;
extern bool reset(void);

/***********************************************************************************************************************
 * GLOBALS
 **********************************************************************************************************************/
static uint32_t g_contentlength = 0;
static ota_update_download_t g_http_ota;

UINT ota_update_progress_rtos = 0;
UINT ota_update_renew_type    = 0;

static UINT g_ota_sflash_addr  = 0;
static UINT g_ota_total_length = 0;

/***********************************************************************************************************************
 * FUNCTION DECLARATIONS
 **********************************************************************************************************************/
UINT app_http_ota_init(UINT fw_type, UINT64 len);
UINT app_http_ota_download(UCHAR *rev_data, UINT rev_data_len);
UINT app_http_ota_renew(void);

/***********************************************************************************************************************
 * HTTP REQUEST
 **********************************************************************************************************************/
http_client_request_t request =
{
    .op_code  = HTTP_CLIENT_OPCODE_GET,
    .port     = PORT,
    .hostname = HOST,

#ifdef HTTPS
    .insecure = pdTRUE,
#else
    .insecure = pdFALSE,
#endif

    .path     = PATH,
};

/***********************************************************************************************************************
 * FIND CONTENT LENGTH
 **********************************************************************************************************************/
static uint32_t find_contentlength(char *str)
{
    char *ptr = strstr(str, "Content-Length:");
    uint32_t len = 0;

    if (ptr)
    {
        sscanf(ptr, "Content-Length: %lu", &len);
    }

    return len;
}

/***********************************************************************************************************************
 * OTA INIT
 **********************************************************************************************************************/
UINT app_http_ota_init(UINT fw_type, UINT64 len)
{
    if (ota_update_check_available_size((ota_update_type)fw_type,
                                        (UINT)len)
        != OTA_SUCCESS)
    {
        APP_PRINT("[OTA] OTA_ERROR_SIZE\r\n");
        return OTA_ERROR_SIZE;
    }

    memset(&g_http_ota, 0, sizeof(g_http_ota));

    g_http_ota.write.sflash_addr =
            ota_update_get_new_sflash_addr(fw_type);

    APP_PRINT("[OTA] Download Address : 0x%08X\r\n",
              g_http_ota.write.sflash_addr);

    g_http_ota.update_type        = fw_type;
    g_http_ota.received_length    = 0;
    g_http_ota.write.total_length = (UINT)len;
    g_http_ota.write.offset       = 0;
    g_http_ota.download_status    = OTA_SUCCESS;
    g_http_ota.version_check      = OTA_NOT_FOUND;
    g_http_ota.content_length     = (UINT)len;

    g_ota_sflash_addr  = g_http_ota.write.sflash_addr;
    g_ota_total_length = (UINT)len;

    return OTA_SUCCESS;
}

/***********************************************************************************************************************
 * OTA DOWNLOAD
 **********************************************************************************************************************/
UINT app_http_ota_download(UCHAR *rev_data, UINT rev_data_len)
{
    UINT status   = OTA_SUCCESS;
    UINT progress = 0;

    if (g_http_ota.update_type != OTA_TYPE_RTOS)
    {
        APP_PRINT("[OTA] OTA_ERROR_TYPE\r\n");
        return OTA_ERROR_TYPE;
    }

    if (rev_data == NULL)
    {
        APP_PRINT("[OTA] rev_data NULL\r\n");
        return OTA_FAILED;
    }

#if defined(DISABLE_OTA_VER_CHK)
    g_http_ota.version_check = OTA_SUCCESS;
#else
    if (g_http_ota.version_check == OTA_NOT_FOUND)
    {
        if (ota_update_check_version(g_http_ota.update_type,
                                     rev_data,
                                     rev_data_len)
            == OTA_SUCCESS)
        {
            g_http_ota.version_check = OTA_SUCCESS;
        }
        else
        {
            APP_PRINT("[OTA] VERSION CHECK FAILED\r\n");
            return OTA_VERSION_INCOMPATI;
        }

        if (ota_update_check_available_size(
                    g_http_ota.update_type,
                    g_http_ota.content_length)
            != OTA_SUCCESS)
        {
            APP_PRINT("[OTA] OTA SIZE INVALID\r\n");
            return OTA_VERSION_INCOMPATI;
        }
    }
#endif

    g_http_ota.received_length += rev_data_len;

    status = ota_update_buffer_write_flash(
                                &g_http_ota.write,
                                rev_data,
                                rev_data_len);

    if (status != OTA_SUCCESS)
    {
        APP_PRINT("[OTA] FLASH WRITE FAILED : 0x%02X\r\n", status);
        return status;
    }

    if ((g_http_ota.received_length > 0) &&
        (g_http_ota.content_length  > 0))
    {
        progress = (g_http_ota.received_length * 100)
                   / g_http_ota.content_length;

        if (ota_update_progress_rtos < progress)
        {
            APP_PRINT("\r[OTA] Downloading... %d %% (%lu/%lu Bytes)",
                      progress,
                      g_http_ota.received_length,
                      g_http_ota.content_length);

            if (progress == 100)
            {
                APP_PRINT("\r\n");
            }
        }

        ota_update_progress_rtos = progress;
    }

    /*******************************************************************
     * DOWNLOAD COMPLETE
     ******************************************************************/
    if (progress == 100)
    {
        APP_PRINT("\r\n");
        APP_PRINT("====================================\r\n");
        APP_PRINT(" OTA DOWNLOAD COMPLETE\r\n");
        APP_PRINT("====================================\r\n");

        if (g_http_ota.version_check == OTA_SUCCESS)
        {
            APP_PRINT("[OTA] version_check OK, writing NVRAM progress...\r\n");

            ota_update_set_download_progress(
                    g_http_ota.update_type, progress);

            ota_update_write_nvram_download_progress(
                    g_http_ota.update_type, progress);

            APP_PRINT("[OTA] NVRAM progress written\r\n");
        }
        else
        {
            APP_PRINT("[OTA] WARNING: version_check NOT OK (0x%02X), NVRAM NOT written!\r\n",
                      g_http_ota.version_check);
        }

        ota_update_print_status(g_http_ota.update_type, OTA_SUCCESS);

        ota_update_renew_type      = g_http_ota.update_type;
        g_http_ota.download_status = OTA_SUCCESS;

        APP_PRINT("[OTA] DOWNLOAD COMPLETE\r\n");

        xTaskNotify(g_app_main_task_handle,
                    TASK_COMPLETE_EVENT,
                    eSetValueWithOverwrite);
    }

    return status;
}

/***********************************************************************************************************************
 * OTA RENEW
 **********************************************************************************************************************/
UINT app_http_ota_renew(void)
{
    UINT status = OTA_SUCCESS;
    OTA_UPDATE_CONFIG *ota_update_conf = NULL;

    ota_update_conf = OTA_MALLOC(sizeof(OTA_UPDATE_CONFIG));

    if (ota_update_conf == NULL)
    {
        APP_PRINT("[OTA] OTA CONFIG ALLOC FAILED\r\n");
        return OTA_FAILED;
    }

    memset(ota_update_conf, 0x00, sizeof(OTA_UPDATE_CONFIG));
    ota_update_conf->renew_notify = NULL;

    APP_PRINT("[OTA] CALLING ota_update_start_renew()\r\n");
    APP_PRINT("[OTA] update_type      = %d\r\n",     ota_update_renew_type);
    APP_PRINT("[OTA] sflash_addr      = 0x%08X\r\n", g_ota_sflash_addr);
    APP_PRINT("[OTA] total_length     = %lu\r\n",    g_ota_total_length);
    APP_PRINT("[OTA] received_length  = %lu\r\n",    g_http_ota.received_length);

    status = ota_update_check_state();

    if (status == OTA_SUCCESS)
    {
        UINT boot_idx_before = ota_update_get_boot_index();
        APP_PRINT("[OTA] Boot index BEFORE renew = %d\r\n", boot_idx_before);

        status = ota_update_current_fw_renew();

        UINT boot_idx_after = ota_update_get_boot_index();
        APP_PRINT("[OTA] Boot index AFTER renew = %d\r\n", boot_idx_after);
    }

    ota_update_status_atcmd(7, status);

    if (status != OTA_SUCCESS)
    {
        APP_PRINT("[OTA] OTA renew failed: 0x%02X\r\n", status);
        OTA_FREE(ota_update_conf);
        return status;
    }

    APP_PRINT("[OTA] OTA renew success\r\n");

    APP_PRINT("[OTA] ==========================================\r\n");
    APP_PRINT("[OTA] OTA complete. Rebooting into Slot 1...\r\n");
    APP_PRINT("[OTA] ==========================================\r\n");

    APP_PRINT("[OTA] Rebooting in 5...\r\n"); vTaskDelay(pdMS_TO_TICKS(1000));
    APP_PRINT("[OTA] Rebooting in 4...\r\n"); vTaskDelay(pdMS_TO_TICKS(1000));
    APP_PRINT("[OTA] Rebooting in 3...\r\n"); vTaskDelay(pdMS_TO_TICKS(1000));
    APP_PRINT("[OTA] Rebooting in 2...\r\n"); vTaskDelay(pdMS_TO_TICKS(1000));
    APP_PRINT("[OTA] Rebooting in 1...\r\n"); vTaskDelay(pdMS_TO_TICKS(1000));

    UINT boot_idx_final = ota_update_get_boot_index();
    APP_PRINT("[OTA] New current boot index value is %d\r\n", boot_idx_final);
    APP_PRINT("[OTA] *** SYSTEM RESET NOW ***\r\n");
    vTaskDelay(pdMS_TO_TICKS(300));

    OTA_FREE(ota_update_conf);

    RM_HTTPS_W_Close(&g_https_w0_ctrl);
    vTaskDelay(pdMS_TO_TICKS(500));

    R_BSP_RetainedMemFlagClear();
    vTaskDelay(pdMS_TO_TICKS(200));

    vTaskDelay(pdMS_TO_TICKS(200));

    SWRESET;

    while (1) { __NOP(); }
    return status;
}

/***********************************************************************************************************************
 * HTTPS CALLBACK
 **********************************************************************************************************************/
void g_https0_callback(https_callback_args_t *p_args)
{
    switch (p_args->event)
    {
        case HTTPS_EVENT_CLIENT_GET_DONE:
        {
            APP_PRINT("\r\n[HTTPS] HEADER RECEIVED\r\n");

            g_contentlength = find_contentlength((char *)p_args->payload);

            APP_PRINT("[HTTPS] Content Length : %lu\r\n", g_contentlength);

            if (app_http_ota_init(OTA_TYPE_RTOS, g_contentlength) != OTA_SUCCESS)
            {
                APP_PRINT("[OTA] INIT FAILED\r\n");
                return;
            }

            g_header_done = 1;
            break;
        }

        case HTTPS_EVENT_CLIENT_RECVED:
        {
            if ((p_args->payload == NULL) || (p_args->len == 0))
            {
                break;
            }

            if (app_http_ota_download((UCHAR *)p_args->payload,
                                       p_args->len) != OTA_SUCCESS)
            {
                APP_PRINT("[OTA] DOWNLOAD FAILED\r\n");
            }

            break;
        }

        case HTTPS_EVENT_CLIENT_RESULT:
        {
            APP_PRINT("\r\n[HTTPS] DOWNLOAD SESSION COMPLETE\r\n");
            break;
        }

        default:
        {
            APP_PRINT("[HTTPS] EVENT : %d\r\n", p_args->event);
            break;
        }
    }
}

/***********************************************************************************************************************
 * OTA START
 **********************************************************************************************************************/
fsp_err_t ota_update_start(void)
{
    fsp_err_t err;

    err = RM_HTTPS_W_Open(&g_https_w0_ctrl, &g_https_w0_cfg);

    if (FSP_SUCCESS != err)
    {
        APP_PRINT("[OTA] HTTPS OPEN FAILED\r\n");
        return err;
    }

    /* Do NOT assign return value — must stay void-called exactly as original */
    RM_HTTPS_W_CallbackSet(&g_https_w0_ctrl, g_https0_callback, NULL, NULL);

    err = RM_HTTPS_W_ClientSendRequest(&g_https_w0_ctrl, &request);

    if (FSP_SUCCESS != err)
    {
        APP_PRINT("[OTA] HTTP REQUEST FAILED\r\n");
        return err;
    }

    return FSP_SUCCESS;
}

/***********************************************************************************************************************
 * HTTP RESTART
 **********************************************************************************************************************/
void http_restart(void)
{
    APP_PRINT("[OTA] HTTP Restart\r\n");
    ota_update_start();
}
