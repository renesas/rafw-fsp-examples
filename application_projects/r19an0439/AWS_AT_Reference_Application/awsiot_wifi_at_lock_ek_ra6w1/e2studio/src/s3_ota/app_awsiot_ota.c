/***********************************************************************************************************************
 * File Name    : app_awsiot_ota.c
 * Description  : Helper functions for aws ota feature support.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "FreeRTOS.h"
#include "custom_config_sdk.h"
/* for __SUPPORT_AWS_IOT_W__*/
#include "rm_awsiot_w_cfg.h"

#if defined (__SUPPORT_OTA__)
#if (DEVICE_FAMILY != DA1640X)
#include "da16x_system.h"
#include "application.h"
#endif

#include "sdk_defs.h"
#include "common_def.h"
#include "ota_update_common.h"
#include "ota_update.h"
#include "app_awsiot_ota.h"
#if (DEVICE_FAMILY != DA1640X)
#include "command.h"
#include "sflash.h"
#endif
#include "common_utils.h"
#include "timers.h"
#include "rm_vee_flash_w_rrq_nvram.h"

UINT ota_update_progress_rtos = 0;
UINT ota_update_progress_ble = 0;
#if defined (__SUPPORT_ATCMD__)
UINT ota_update_progress_mcu = 0;
#endif
UINT ota_update_renew_type = 0;
static ota_update_download_t awsiot;

UINT app_awsiot_ota_init(UINT fw_type, UINT64 len)
{
    if (ota_update_check_available_size((ota_update_type)fw_type, (UINT)len) != OTA_SUCCESS)
    {
        return OTA_ERROR_SIZE;
    }

    awsiot.write.sflash_addr = ota_update_get_new_sflash_addr(fw_type);
#if (DEVICE_FAMILY == DA1640X)
    if ((awsiot.write.sflash_addr != SF_RTOS_0)
        && (awsiot.write.sflash_addr != SF_RTOS_1))
#else
    if ((awsiot.write.sflash_addr != OTA_STOR_RTOS_0_ADDR)
        && (awsiot.write.sflash_addr != OTA_STOR_RTOS_1_ADDR))
#endif
    {
        PRINTF("[%s:%d] OTA_ERROR_SFLASH_ADDR\n",__func__,__LINE__);
        return OTA_ERROR_SFLASH_ADDR;
    }    

    awsiot.update_type = fw_type;
    awsiot.received_length = 0;
    awsiot.write.total_length = (UINT)len;
    awsiot.write.offset = 0;
    awsiot.download_status = OTA_SUCCESS;
    awsiot.version_check = OTA_NOT_FOUND;
    awsiot.content_length = (UINT)len;
    awsiot.received_length = 0;

    return OTA_SUCCESS;
}

 UINT app_awsiot_ota_download(UCHAR *rev_data, UINT rev_data_len)
 {
    UINT status = OTA_SUCCESS;
    UINT progress = 0;

    if (awsiot.update_type != OTA_TYPE_RTOS)
    {
        status = OTA_ERROR_TYPE;
        PRINTF("[%s:%d] OTA_ERROR_SFLASH_ADDR\n",__func__,__LINE__);
        goto finish;
    }

    if (rev_data == NULL)
    {
        PRINTF("[%s] rev_data is null\n", __func__);
        status = OTA_FAILED;
        goto finish;

    }
    else
    {

#if defined (DISABLE_OTA_VER_CHK)
        awsiot.version_check = OTA_SUCCESS;
#else
        if (awsiot.version_check == OTA_NOT_FOUND)
        {
            if (ota_update_check_version(awsiot.update_type,
                                         rev_data, rev_data_len) == OTA_SUCCESS)
            {
                awsiot.version_check = OTA_SUCCESS;
            }
            else
            {
                /* Version mismatch */
                awsiot.version_check = OTA_VERSION_INCOMPATI;
                status = OTA_VERSION_INCOMPATI;
                PRINTF("[%s:%d] OTA_VERSION_INCOMPATI\n",__func__,__LINE__);
                goto finish;
            }

            if (awsiot.version_check == OTA_SUCCESS)
            {
                if (ota_update_check_available_size(awsiot.update_type,
                                                    awsiot.content_length) != OTA_SUCCESS)
                {
                    awsiot.version_check = OTA_VERSION_INCOMPATI;
                    status = OTA_VERSION_INCOMPATI;
                    PRINTF("[%s:%d] OTA_VERSION_INCOMPATI\n",__func__,__LINE__);
                    goto finish;
                }

            }
        }
#endif // (DISABLE_OTA_VER_CHK)

        awsiot.received_length += rev_data_len;
        status = ota_update_buffer_write_flash(&awsiot.write, rev_data, rev_data_len);

        if (status != OTA_SUCCESS)
        {
            PRINTF("[%s] Failed to write data to sflash(0x%02x)\n", __func__, status);
            goto finish;
        }

        if ((awsiot.received_length > 0) && (awsiot.content_length > 0))
        {
            progress = (awsiot.received_length * 100) / awsiot.content_length;
            if (awsiot.update_type == OTA_TYPE_RTOS)
            {
                 if (ota_update_progress_rtos< progress)
                 {
                     PRINTF("\r   >> [sflash_addr:0x%x] RTOS Downloading... %d %% (%d/%d Bytes)%s\n",
                     awsiot.write.sflash_addr,
                     progress,
                     awsiot.received_length,
                     awsiot.content_length, progress == 100 ? "\n" : " ");
                 }
                 ota_update_progress_rtos  = progress;
#if defined (__SUPPORT_ATCMD__)
            }
            else if (awsiot.update_type == OTA_TYPE_MCU_FW_STREAM)
            {
                 if (ota_update_progress_mcu< progress)
                 {
                     PRINTF("\r   >> [sflash_addr:0x%x] MCU Downloading... %d %% (%d/%d Bytes)%s\n",
                     awsiot.write.sflash_addr,
                     progress,
                     awsiot.received_length,
                     awsiot.content_length, progress == 100 ? "\n" : " ");
                 }
                 ota_update_progress_mcu  = progress;
#endif // (__SUPPORT_ATCMD__)
            }
        }
        else
        {
            progress = 0;
        }
    }

finish:
    if ((status != OTA_SUCCESS) || progress == 100)
    {
        if (progress == 100)
        {
             if (awsiot.version_check == OTA_SUCCESS)
             {
                 ota_update_set_download_progress(awsiot.update_type, progress);
                 ota_update_write_nvram_download_progress(awsiot.update_type, progress);
             }
             ota_update_print_status(awsiot.update_type, OTA_SUCCESS);
             ota_update_renew_type = awsiot.update_type;
             awsiot.update_type = OTA_TYPE_INIT;
             awsiot.received_length = 0;
             awsiot.write.sflash_addr = 0;
             awsiot.write.total_length = 0;
             awsiot.write.offset = 0;
             if (ota_update_renew_type == OTA_TYPE_RTOS)
             {
                 awsiot.download_status = OTA_SUCCESS;
#if defined (__SUPPORT_ATCMD__)
             }
             else if (ota_update_renew_type == OTA_TYPE_MCU_FW_STREAM)
             {
                 awsiot.download_status = OTA_SUCCESS;
#endif // (__SUPPORT_ATCMD__)
             }
             awsiot.version_check = OTA_NOT_FOUND;
             awsiot.content_length = 0;
             awsiot.received_length = 0;
        }
        PRINTF("status:0x%02x\r\n", status);
    }

    return status;
}

 UINT app_awsiot_ota_renew(void)
{
    UINT status = OTA_SUCCESS;
    OTA_UPDATE_CONFIG *ota_update_conf = NULL;

#if defined (__SUPPORT_ATCMD__)
    if (ota_update_renew_type == OTA_TYPE_MCU_FW_STREAM)
        return status;
#endif  //(__SUPPORT_ATCMD__)
    ota_update_conf = OTA_MALLOC(sizeof(OTA_UPDATE_CONFIG));

    if (ota_update_conf == NULL)
    {
        PRINTF("[%s] Failed to alloc memory\n", __func__);
        return OTA_FAILED;
    }

    memset(ota_update_conf, 0x00, sizeof(OTA_UPDATE_CONFIG));
    ota_update_conf->renew_notify = NULL;
    status = ota_update_start_renew(ota_update_conf);
    if (ota_update_conf != NULL) {
        OTA_FREE(ota_update_conf);
    }

    return status;
}

#if defined (__IMG_UPDATE_BY_MCU__)
#define OTA_REV_ACK_TIMEOUT 5000

static ota_update_download_t awsiot_mcu;
static TimerHandle_t            ota_rev_ack_timer = NULL;

VOID ota_update_direct_write_timeout(void *arg)
{
#if (DEVICE_FAMILY == DA1640X)
    RA6W1_UNUSED_ARG(arg);
#else
    DA16X_UNUSED_ARG(arg);
#endif
    atcmd_asynchony_event(11, NULL); //+FWUPDATE
    awsiot_mcu.download_status = OTA_STATE_READY;
    awsiot_mcu.update_type = MATTER_OTA_INIT;
    PRINTF("[%s] byMCU Write timeout\n", __func__);

    return;
}

UINT matter_ota_update_by_mcu_init(UINT fw_type, UINT len)
{
    int ret;

    if (ota_update_check_accept_size(fw_type, len) != OTA_SUCCESS)
    {
        return OTA_ERROR_SIZE;
    }

    awsiot_mcu.write.sflash_addr = ota_update_get_new_sflash_addr(fw_type);
    if ((awsiot_mcu.write.sflash_addr != OTA_STOR_RTOS_0_ADDR)
        && (awsiot_mcu.write.sflash_addr != OTA_STOR_RTOS_1_ADDR))
    {
        return OTA_ERROR_SFLASH_ADDR;
    }

    awsiot_mcu.update_type = fw_type;
    awsiot_mcu.received_length = 0;
    awsiot_mcu.write.total_length = len;
    awsiot_mcu.write.offset = 0;
    awsiot_mcu.download_status = OTA_STATE_READY;
    awsiot_mcu.version_check = OTA_NOT_FOUND;
    awsiot_mcu.content_length = len;
    awsiot_mcu.received_length = 0;

    if (ota_rev_ack_timer != NULL)
    {
        xTimerStop(ota_rev_ack_timer, 0);
        xTimerDelete(ota_rev_ack_timer, 0);
        memset(&ota_rev_ack_timer, 0, sizeof(TimerHandle_t));
    }

    /* Create timer to trigger the dpm_sleep_daemon periodically. */
    ota_rev_ack_timer = xTimerCreate("ota_rev_ack_timer",
                                     pdMS_TO_TICKS(OTA_REV_ACK_TIMEOUT),
                                     pdFALSE,
                                     (void *)NULL,
                                     (TimerCallbackFunction_t) ota_update_direct_write_timeout);
    if (ota_rev_ack_timer == NULL)
    {
        PRINTF("[%s] Failed to create ota_rev_ack_timer !!!\n", __func__);

        return OTA_FAILED;
    }

    ret = xTimerStart(ota_rev_ack_timer, 0);
    if (ret != pdPASS)
    {
        PRINTF("[%s] Failed to start timer(%d)\r\n", __func__, ret);
        xTimerDelete(ota_rev_ack_timer, 0);

        return OTA_FAILED;
    }

    return OTA_SUCCESS;
}

UINT matter_ota_update_by_mcu_download(UCHAR *rev_data, UINT rev_data_len)
{
    UINT status = OTA_SUCCESS;
    UINT progress = 0;

    if (awsiot_mcu.update_type != OTA_TYPE_RTOS)
    {
        status = OTA_ERROR_TYPE;
        goto finish;
    }

    if ((awsiot_mcu.write.sflash_addr < ota_update_get_new_sflash_addr(OTA_TYPE_RTOS))
        || (awsiot_mcu.write.sflash_addr > (ota_update_get_new_sflash_addr(OTA_TYPE_RTOS) + awsiot_mcu.content_length))){
        status = OTA_ERROR_SFLASH_ADDR;
        goto finish;
    }

    if (ota_rev_ack_timer != NULL)
    {
        xTimerReset(ota_rev_ack_timer, 0);
    }
    else
    {
        status = OTA_FAILED_TIMER;
        goto finish;
    }

    awsiot_mcu.download_status = OTA_STATE_PROGRESS;

    if (rev_data == NULL)
    {
        PRINTF("[%s] rev_data is null\n", __func__);
        status = OTA_FAILED;
        goto finish;
    }
    else
    {
#if defined (DISABLE_OTA_VER_CHK)
        awsiot_mcu.version_check = OTA_SUCCESS;
#else
        if (awsiot_mcu.version_check == OTA_NOT_FOUND)
        {
            if (ota_update_check_version(awsiot_mcu.update_type,
                rev_data, rev_data_len) == OTA_SUCCESS)
            {
                awsiot_mcu.version_check = OTA_SUCCESS;
            }
            else
            {
                /* Version mismatch */
                awsiot_mcu.version_check = OTA_VERSION_INCOMPATI;
                status = OTA_VERSION_INCOMPATI;
                goto finish;
            }

            if (awsiot_mcu.version_check == OTA_SUCCESS)
            {
                if (ota_update_check_available_size(awsiot_mcu.update_type,
                                                    awsiot_mcu.content_length) != OTA_SUCCESS)
                {
                    awsiot_mcu.version_check = OTA_VERSION_INCOMPATI;
                    status = OTA_VERSION_INCOMPATI;
                    goto finish;
                }
            }
        }
#endif // (DISABLE_OTA_VER_CHK)

        awsiot_mcu.received_length += rev_data_len;
        status = ota_update_buffer_write_sflash(&awsiot_mcu.write, rev_data, rev_data_len);

        if (status != OTA_SUCCESS)
        {
            PRINTF("[%s] Failed to write data to sflash(0x%02x)\n", __func__, status);
            goto finish;
        }

        if ((awsiot_mcu.received_length > 0) && (awsiot_mcu.content_length > 0))
        {
            progress = (awsiot_mcu.received_length * 100) / awsiot_mcu.content_length;
            PRINTF("\r   >> By MCU Downloading... %d %% (%d/%d Bytes)%s",
                progress,
                awsiot_mcu.received_length,
                awsiot_mcu.content_length, progress == 100 ? "\n" : " ");
            ota_update_progress_rtos  = progress;
        }
        else
        {
            progress = 0;
        }
    }

finish:
    PRINTF_ATCMD("\r\n+FWUPDATE:0x%02x\r\n", status);

    if ((status != OTA_SUCCESS) || progress == 100)
    {
        xTimerStop(ota_rev_ack_timer, 0);
        xTimerDelete(ota_rev_ack_timer, 0);
        memset(&ota_rev_ack_timer, 0, sizeof(TimerHandle_t));

        if (progress == 100)
        {
            ota_update_print_status(awsiot_mcu.update_type, OTA_SUCCESS);
            awsiot_mcu.received_length = 0;
            awsiot_mcu.write.sflash_addr = 0;
            awsiot_mcu.write.total_length = 0;
            awsiot_mcu.write.offset = 0;
            awsiot_mcu.version_check = OTA_NOT_FOUND;
            awsiot_mcu.content_length = 0;
            awsiot_mcu.received_length = 0;
            ota_update_renew_type = awsiot_mcu.update_type;
        }
        awsiot_mcu.update_type = OTA_TYPE_INIT;
        awsiot_mcu.download_status = OTA_STATE_READY;
        PRINTF_ATCMD("\r\n+NWOTABYMCU:0x%02x\r\n", status);
    }

    return status;
}
#endif // (__IMG_UPDATE_BY_MCU__)
#endif // (__SUPPORT_OTA__)
