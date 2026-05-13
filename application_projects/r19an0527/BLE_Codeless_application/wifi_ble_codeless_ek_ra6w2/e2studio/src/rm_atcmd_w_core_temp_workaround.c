/*
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/***********************************************************************************************************************
 * Includes   <System Includes> , "Project Includes"
 **********************************************************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <stdlib.h>
#include "rm_atcmd_w_core.h"
#include "rm_atcmd_w_core_err_code.h"

#if (ATCMD_DA14XXX_CODELESS == 1)
 #include "rm_atcmd_w_core_da14xxx_parse.h"
 #include "common_data.h"
#endif

#if (ATCMD_IF_SUPPORT == 1)
 #include "FreeRTOS.h"

 #include "rm_atcmd_w_core_network_parse.h"
 #include "rm_atcmd_w_core_mqtt_parse.h"
 #include "rm_atcmd_w_core_basic_parse.h"
 #include "rm_atcmd_w_core_wifi_parse.h"
 #include "rm_atcmd_w_core_socket_parse.h"
 #include "rm_atcmd_w_core_websocket_parse.h"
 #include "rm_atcmd_w_core_http_parse.h"
 #include "rm_atcmd_w_core_ota_parse.h"
 #include "rm_atcmd_w_core_dpm_parse.h"
 #include "rm_atcmd_w_core_prod_test_parse.h"
 #include "rm_atcmd_w_core_pin_port.h"

 #if (ATCMD_RF_TEST_SUPPORT == 1)
  #include "rm_atcmd_w_core_rf_test_parse.h"
 #endif

 #if (ATCMD_SECURE_CHANNEL == 1)
  #include "rm_atcmd_transport_uart_w.h"
 #endif

 #if (__SUPPORT_MATTER_IOT__)
  #include "rm_atcmd_w_core_matter_parse.h"
 #endif

 #include "FreeRTOS.h"
 #include "task.h"
 #include "semphr.h"
 #include "stream_buffer.h"
 #include "event_groups.h"
 #if CFG_WIFI
  #include "romac4rtos.h"
  #include "common_def.h"
  #include "rm_wifi_dpm.h"
 #endif                                /* CFG_WIFI */
 #if CFG_PMGR
  #include "rm_pmgr_w_instance.h"
  #include "rm_atcmd_w_core_http.h"
 #endif
#endif

#if (ATCMD_IF_SUPPORT == 1)
static atcmd_w_core_module_t         * at_core_user_cmd_module         = NULL;
static atcmd_w_core_unfixed_module_t * at_core_unfixed_user_cmd_module = NULL;
ATCmdOpenCallback_t atcmd_open_user_app_callback  = NULL;
ATCmdOpenCallback_t atcmd_close_user_app_callback = NULL;
#endif

#if (ATCMD_SECURE_CHANNEL == 1)
 #define HDR_LEN              (3U)
 #define SEC_TX_PREFIX_MAX    (16)     /* "\r\n+SEC:<len>," */
#endif

/***********************************************************************************************************************
 * Exported global variables (to be accessed by other files)
 **********************************************************************************************************************/
atcmd_w_uart_input_buff_t at_cmd_input_buff;
uint16_t atcmd_echo_on;

/***********************************************************************************************************************
 * Function Prototypes
 **********************************************************************************************************************/
#if (ATCMD_IF_SUPPORT == 1)
static EventGroupHandle_t init_rsp_evt_grp = NULL;
 #if ATCMD_ASSUME_MCU_ALWAYS_ON
  #define DEF_MCU_WAKEUP_DONE    (1)
 #else
  #define DEF_MCU_WAKEUP_DONE    (0)
 #endif                                /* ASSUME_MCU_ALWAYS_ON */

 #if (ATCMD_DA14XXX_CODELESS == 1)
TaskHandle_t rm_atcmd_w_core_task_hdl = NULL;
 #endif
static TaskHandle_t       rm_atcmd_w_core_parser_task     = NULL;
static const unsigned int rm_atcmd_w_core_paser_task_size = (1024 * 12 / 4);

static int                    rm_atcmd_w_core_wait_ready(atcmd_w_ctrl_t * const p_at_ctrl, uint32_t timeout_ms);
static fsp_err_atcmd_err_code rm_atcmd_w_core_common_parser(atcmd_w_ctrl_t * const       p_at_ctrl,
                                                            atcmd_w_core_module_list_t * p_list,
                                                            atcmd_w_uart_input_buff_t  * p_in);
static void      rm_atcmd_w_core_parser_main(void * pvParameters);
static fsp_err_t rm_atcmd_w_core_init_parser(atcmd_w_ctrl_t * const p_at_ctrl);

#endif
static fsp_err_t rm_atcmd_w_core_write(atcmd_w_ctrl_t * const p_at_ctrl,
                                       uint8_t const * const  p_src,
                                       uint32_t const         bytes);

#if (ATCMD_SECURE_CHANNEL == 1)
static fsp_err_t rm_atcmd_w_core_secchan_write(atcmd_w_ctrl_t * const p_at_ctrl,
                                               uint8_t const * const  p_src,
                                               uint32_t const         bytes);

#endif

/* AT HAL API mapping for AT interface */
const atcmd_w_api_t g_at_core =
{
    .open               = RM_ATCMD_W_CORE_Open,
    .close              = RM_ATCMD_W_CORE_Close,
    .write              = RM_ATCMD_W_CORE_Write,
    .read               = RM_ATCMD_W_CORE_Read,
    .infoGet            = RM_ATCMD_W_CORE_InfoGet,
    .communicationAbort = RM_ATCMD_W_CORE_Abort,
    .callbackSet        = RM_ATCMD_W_CORE_CallbackSet,
    .readStop           = RM_ATCMD_W_CORE_ReadStop,
};

/*******************************************************************************************************************//**
 * @addtogroup AT
 * @{
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * This functions initializes the AT communication module. Implements @ref atcmd_w_api_t::open.
 *
 * This function performs the following tasks:
 * - Performs parameter checking and processes error conditions.
 * - Configures the peripheral registers according to the configuration.
 * - Initialize the control structure for use in other @ref AT_API functions.
 *
 * @retval     FSP_SUCCESS                     Module initialized successfully.
 * @retval     FSP_ERR_ALREADY_OPEN            Instance was already initialized.
 * @return     See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 *             This function calls: @ref transfer_api_t::open
 * @note       This function is reentrant.
 **********************************************************************************************************************/
fsp_err_t RM_ATCMD_W_CORE_Open (atcmd_w_ctrl_t * const p_at_ctrl, atcmd_w_cfg_t const * const p_cfg)
{
    atcmd_w_core_instance_ctrl_t * p_ctrl = (atcmd_w_core_instance_ctrl_t *) p_at_ctrl;

#if (ATCMD_W_CFG_PARAM_CHECKING_ENABLE)

    /* Check parameters. */
    FSP_ASSERT(p_ctrl);
    FSP_ASSERT(p_cfg);
    FSP_ERROR_RETURN(ATCMD_W_CORE_OPEN_KEY != p_ctrl->open, FSP_ERR_ALREADY_OPEN);
#endif

    p_ctrl->open = ATCMD_W_CORE_OPEN_KEY;

    p_ctrl->p_cfg             = p_cfg;
    p_ctrl->p_callback        = p_cfg->p_callback;
    p_ctrl->p_context         = p_cfg->p_context;
    p_ctrl->p_callback_memory = NULL;
    p_ctrl->p_tx_src          = NULL;
    p_ctrl->tx_src_bytes      = 0U;
    p_ctrl->rx_dest_bytes     = 0;
    p_ctrl->rx_dest_idx       = 0;

    p_ctrl->p_transport_instance = (atcmd_transport_w_instance_t *) p_cfg->p_context;

#if (ATCMD_SECURE_CHANNEL == 1)
    p_ctrl->ctx_rx         = &(p_ctrl->ctx_rx_body);
    p_ctrl->ctx_tx         = &(p_ctrl->ctx_tx_body);
    p_ctrl->secure_channel = 0;
#endif

#if (ATCMD_W_CORE_ECHO_EN)
    memset(at_cmd_input_buff.at_cmd_resp_str, 0x00, ATCMD_W_RESP_LEN_MAX);
#endif
    memset(at_cmd_input_buff.at_cmd_req_str, 0x00, ATCMD_W_RESP_LEN_MAX);
#if (ATCMD_SECURE_CHANNEL == 1)
    memset(p_ctrl->key, 0, ATCMD_W_SECURE_KEY_MAX);
#endif

    at_cmd_input_buff.at_cmd_req_idx = 0;

#if (ATCMD_IF_SUPPORT == 1)
    rm_atcmd_w_core_init_parser(p_at_ctrl);

 #if CFG_PMGR
    if (RM_PMGR_W_dpm_is_wakeup())
    {
        unsigned long long server_en = 0;

        RM_PMGR_W_rtm_static_get(RTM_STATIC_KEY_HTTP_SVR_ENABLE_FLAG, NULL, &server_en, NULL);

        if (server_en)
        {
            char * _cmd[2];
            _cmd[0] = "http-server";
            _cmd[1] = "start";

            rm_atcmd_w_run_user_http_server(p_at_ctrl, 2, _cmd);
        }
    }
 #endif
#endif

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * This function receives data from a AT transport layer. Implements @ref atcmd_w_api_t::read.
 *
 * The function performs the following tasks:
 * - Performs parameter checking and processes error conditions.
 * - Sets up the instance to complete a AT read operation.
 *
 * @retval  FSP_SUCCESS                   Read operation successfully completed.
 * @retval  FSP_ERR_ASSERTION             NULL pointer to control or destination parameters or transfer length is zero.
 * @retval  FSP_ERR_NOT_OPEN              The transport has not been opened. Open transport first.
 * @retval  FSP_ERR_IN_USE                A transfer is already in progress.
 **********************************************************************************************************************/
fsp_err_t RM_ATCMD_W_CORE_Read (atcmd_w_ctrl_t * const p_at_ctrl, uint8_t * const p_dest, uint32_t const bytes)
{
#if (ATCMD_W_CFG_PARAM_CHECKING_ENABLE)
    FSP_ASSERT(p_at_ctrl);
    FSP_ASSERT(p_dest);
    FSP_ASSERT(bytes > 0);
#endif

#if (ATCMD_IF_SUPPORT == 1)
    atcmd_w_core_instance_ctrl_t * p_ctrl = (atcmd_w_core_instance_ctrl_t *) p_at_ctrl;
    uint32_t timeout   = portMAX_DELAY;
    uint32_t available = p_ctrl->rx_dest_bytes - p_ctrl->rx_dest_idx;
    uint32_t to_read   = bytes;

    while (0 != to_read)
    {
        uint32_t copy_bytes = MIN(to_read, available);

        if (0 != copy_bytes)
        {
            memcpy(p_dest + (bytes - to_read), p_ctrl->rx_dest + p_ctrl->rx_dest_idx, copy_bytes);

            p_ctrl->rx_dest_idx += copy_bytes;
            to_read             -= copy_bytes;
        }
        else
        {
            size_t ret =
                p_ctrl->p_transport_instance->p_api->bufferRecv(p_ctrl->p_transport_instance->p_ctrl,
                                                                (const char *) p_ctrl->rx_dest,
                                                                sizeof(p_ctrl->rx_dest),
                                                                timeout);
            p_ctrl->rx_dest_bytes = ret;
            p_ctrl->rx_dest_idx   = 0;

            if (0 == ret)
            {
                return FSP_ERR_TIMEOUT;
            }

            available = p_ctrl->rx_dest_bytes - p_ctrl->rx_dest_idx;
        }
    }

    return FSP_SUCCESS;
#else

    return FSP_SUCCESS;
#endif
}

static fsp_err_t rm_atcmd_w_core_write (atcmd_w_ctrl_t * const p_at_ctrl,
                                        uint8_t const * const  p_src,
                                        uint32_t const         bytes)
{
    fsp_err_t err = FSP_SUCCESS;
    atcmd_w_core_instance_ctrl_t * p_ctrl = (atcmd_w_core_instance_ctrl_t *) p_at_ctrl;
    atcmd_transport_w_data_t       p_at_transport_data;
    atcmd_transport_w_status_t     p_status;

#if (ATCMD_IF_SUPPORT == 1)
    if (rm_atcmd_w_core_wait_ready(p_at_ctrl, 1000 /* ms */))
    {
        return FSP_ERR_NOT_INITIALIZED;
    }
#endif

    p_at_transport_data.at_cmd_string_length = bytes;
    p_at_transport_data.p_at_cmd_string      = (uint8_t *) p_src;
    p_ctrl->p_transport_instance->p_api->statusGet(p_ctrl->p_transport_instance->p_ctrl, &p_status);

    if (p_status.open)
    {
        err = p_ctrl->p_transport_instance->p_api->atCommandSend(p_ctrl->p_transport_instance->p_ctrl,
                                                                 &p_at_transport_data);
    }

    return err;
}

#if (ATCMD_SECURE_CHANNEL == 1)
static int is_valid_hex_str (const char * s, size_t len)
{
    size_t i;

    if (!s || (len == 0U) || ((len & 1U) != 0U))
    {
        return 0;                      /* must be even length */
    }

    for (i = 0; i < len; i++)
    {
        if (!is_hex_char(s[i]))
        {
            return 0;
        }
    }

    return 1;
}

static fsp_err_t rm_atcmd_w_core_secchan_write (atcmd_w_ctrl_t * const p_at_ctrl,
                                                uint8_t const * const  p_src,
                                                uint32_t const         bytes)
{
    fsp_err_t err = FSP_SUCCESS;
    atcmd_w_core_instance_ctrl_t * p_ctrl = (atcmd_w_core_instance_ctrl_t *) p_at_ctrl;
    char    * p_resp = NULL;
    int       padded;
    uint32_t  resp_sz;
    uint8_t * cipher     = NULL;
    uint8_t * cipher_new = NULL;
    char      prefix[SEC_TX_PREFIX_MAX];
    int       prefix_len;

    p_resp = pvPortMalloc(bytes + AES_BLOCK_SIZE + SEC_TX_PREFIX_MAX);

    if (p_resp == NULL)
    {
        return FSP_ERR_WRITE_FAILED;
    }

    memcpy(p_resp, p_src, bytes);

    if (bytes <= HDR_LEN)
    {
        err = FSP_ERR_WRITE_FAILED;
        goto rm_atcmd_w_core_secchan_write_done;
    }

    cipher = (uint8_t *) p_resp + HDR_LEN;
    padded = rm_atcmd_secchan_pad((char *) cipher, (int) (bytes - HDR_LEN));

    if (padded <= 0)
    {
        err = FSP_ERR_WRITE_FAILED;
        goto rm_atcmd_w_core_secchan_write_done;
    }

    resp_sz = (uint32_t) padded;

    int ret =
        mbedtls_aes_crypt_cbc(p_ctrl->ctx_tx,
                              MBEDTLS_AES_ENCRYPT,
                              (size_t) resp_sz,
                              p_ctrl->tx_iv,
                              (const uint8_t *) cipher,
                              cipher);

    if (ret != 0)
    {
        err = FSP_ERR_WRITE_FAILED;
        goto rm_atcmd_w_core_secchan_write_done;
    }

    /* Build "\r\n+SEC:<len>," */
    prefix_len = snprintf(prefix, sizeof(prefix), "\r\n+SEC:%lu,", (unsigned long) resp_sz);

    if ((prefix_len <= 0) || (prefix_len >= (int) sizeof(prefix)))
    {
        err = FSP_ERR_WRITE_FAILED;
        goto rm_atcmd_w_core_secchan_write_done;
    }

    /* Move ciphertext forward */
    cipher_new = (uint8_t *) p_resp + prefix_len;
    memmove(cipher_new, cipher, resp_sz);
    memcpy(p_resp, prefix, (size_t) prefix_len);
    err = rm_atcmd_w_core_write(p_at_ctrl, (uint8_t *) p_resp, (uint32_t) prefix_len + resp_sz);

rm_atcmd_w_core_secchan_write_done:
    if (p_resp != NULL)
    {
        vPortFree(p_resp);
    }

    return err;
}

#endif

/*******************************************************************************************************************//**
 * This function transmits data to a AT transport layer. Implements @ref atcmd_w_api_t::write.
 *
 * The function performs the following tasks:
 * - Performs parameter checking and processes error conditions.
 * - Sets up the instance to complete a AT write operation.
 *
 * @retval  FSP_SUCCESS                     Write operation successfully completed.
 * @retval  FSP_ERR_ASSERTION               NULL pointer to control or source parameters or transfer length is zero.
 * @retval  FSP_ERR_NOT_OPEN                The transport has not been opened. Open the transport first.
 * @retval  FSP_ERR_IN_USE                  A transfer is already in progress.
 **********************************************************************************************************************/
fsp_err_t RM_ATCMD_W_CORE_Write (atcmd_w_ctrl_t * const p_at_ctrl, uint8_t const * const p_src, uint32_t const bytes)
{
#if (ATCMD_W_CFG_PARAM_CHECKING_ENABLE)
    FSP_ASSERT(p_at_ctrl);
    FSP_ASSERT(p_src);
    FSP_ASSERT(bytes > 0);
#endif

#if (ATCMD_SECURE_CHANNEL == 1)
    atcmd_w_core_instance_ctrl_t * p_ctrl = (atcmd_w_core_instance_ctrl_t *) p_at_ctrl;
    if ((p_ctrl->secure_channel == 1) && (strncmp((const char *) p_src, "\r\n+", 3) == 0))
    {
        return rm_atcmd_w_core_secchan_write(p_at_ctrl, p_src, bytes);
    }
#endif

    return rm_atcmd_w_core_write(p_at_ctrl, p_src, bytes);
}

/*******************************************************************************************************************/ /**
 * Provides the middleware information, including the maximum number of bytes that can be received or transmitted at
 * a time.
 * Implements @ref atcmd_w_api_t::infoGet
 * @retval  FSP_SUCCESS                  Information stored in provided p_info.
 * @retval  FSP_ERR_ASSERTION            Pointer to AT control block is NULL.
 * @retval  FSP_ERR_NOT_OPEN             The control block has not been opened
 **********************************************************************************************************************/
fsp_err_t RM_ATCMD_W_CORE_InfoGet (atcmd_w_ctrl_t * const p_at_ctrl, atcmd_w_info_t * const p_info)
{
#if (ATCMD_W_CFG_PARAM_CHECKING_ENABLE)
    atcmd_w_core_instance_ctrl_t * p_ctrl = (atcmd_w_core_instance_ctrl_t *) p_at_ctrl;
#else
    FSP_PARAMETER_NOT_USED(p_at_ctrl);
    FSP_PARAMETER_NOT_USED(p_info);
#endif

#if (ATCMD_W_CFG_PARAM_CHECKING_ENABLE)
    FSP_ASSERT(p_ctrl);
    FSP_ASSERT(p_info);
    FSP_ERROR_RETURN(ATCMD_W_CORE_OPEN_KEY == p_ctrl->open, FSP_ERR_NOT_OPEN);
#endif

    p_info->read_bytes_max  = ATCMD_W_RESP_LEN_MAX;
    p_info->write_bytes_max = ATCMD_W_TX_QUEUE_SIZE;

    return FSP_SUCCESS;
}

/*******************************************************************************************************************/ /**
 * Provides API to abort ongoing transfer. Transmission is aborted after the current character is transmitted.
 * Reception is still enabled after abort(). Any characters received after abort() and before the transfer
 * is reset in the next call to read(), will arrive via the callback function.
 * Implements @ref atcmd_w_api_t::communicationAbort
 *
 * @retval  FSP_SUCCESS                  AT transaction aborted successfully.
 * @retval  FSP_ERR_ASSERTION            Pointer to AT control block is NULL.
 * @retval  FSP_ERR_NOT_OPEN             The control block has not been opened.
 * @retval  FSP_ERR_UNSUPPORTED          The requested Abort direction is unsupported.
 *
 * @return                       See @ref RENESAS_ERROR_CODES or functions called by this function for other possible
 *                               return codes. This function calls: @ref transfer_api_t::disable
 **********************************************************************************************************************/
fsp_err_t RM_ATCMD_W_CORE_Abort (atcmd_w_ctrl_t * const p_at_ctrl)
{
    FSP_PARAMETER_NOT_USED(p_at_ctrl);

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * Updates the user callback and has option of providing memory for callback structure.
 * Implements atcmd_w_api_t::callbackSet
 *
 * @retval  FSP_SUCCESS                  Callback updated successfully.
 * @retval  FSP_ERR_ASSERTION            A required pointer is NULL.
 * @retval  FSP_ERR_NOT_OPEN             The control block has not been opened.
 **********************************************************************************************************************/
fsp_err_t RM_ATCMD_W_CORE_CallbackSet (atcmd_w_ctrl_t * const          p_at_ctrl,
                                       void (                        * p_callback)(atcmd_w_callback_args_t *),
                                       void const * const              p_context,
                                       atcmd_w_callback_args_t * const p_callback_memory)
{
    FSP_PARAMETER_NOT_USED(p_at_ctrl);
    FSP_PARAMETER_NOT_USED(p_callback);
    FSP_PARAMETER_NOT_USED(p_context);
    FSP_PARAMETER_NOT_USED(p_callback_memory);

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * This function manages the closing of the module by the following task. Implements @ref atcmd_w_api_t::close.
 *
 * Disables AT operations by disabling the selected communication bus (UART,SPI or SDIO).
 * - Disables the AT peripheral.
 * - Disables all the associated interrupts.
 * - Update control structure so it will not work with @ref AT_API functions.
 *
 * @retval  FSP_SUCCESS              Module successfully closed.
 * @retval  FSP_ERR_ASSERTION        A required pointer argument is NULL.
 * @retval  FSP_ERR_NOT_OPEN         The module has not been opened. Open the module first.
 **********************************************************************************************************************/
fsp_err_t RM_ATCMD_W_CORE_Close (atcmd_w_ctrl_t * const p_at_ctrl)
{
    atcmd_w_core_instance_ctrl_t * p_ctrl = (atcmd_w_core_instance_ctrl_t *) p_at_ctrl;

#if (ATCMD_W_CFG_PARAM_CHECKING_ENABLE)
    FSP_ASSERT(p_ctrl);
    FSP_ERROR_RETURN(ATCMD_W_CORE_OPEN_KEY == p_ctrl->open, FSP_ERR_NOT_OPEN);
#endif

    p_ctrl->p_cfg             = NULL;
    p_ctrl->p_callback        = NULL;
    p_ctrl->p_context         = NULL;
    p_ctrl->p_callback_memory = NULL;
    p_ctrl->p_tx_src          = NULL;
    p_ctrl->rx_dest_bytes     = 0;
    p_ctrl->rx_dest_idx       = 0;

    p_ctrl->p_transport_instance = NULL;

#if (ATCMD_W_CORE_ECHO_EN)
    memset(at_cmd_input_buff.at_cmd_resp_str, 0x00, ATCMD_W_RESP_LEN_MAX);
#endif
    memset(at_cmd_input_buff.at_cmd_req_str, 0x00, ATCMD_W_RESP_LEN_MAX);
    at_cmd_input_buff.at_cmd_req_idx = 0;

    p_ctrl->open = ATCMD_W_CORE_CLOSE_KEY;

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * Provides API to abort ongoing read. Reception is still enabled after abort(). Any characters received after abort()
 * and before the transfer is reset in the next call to read().
 * Implements @ref atcmd_w_api_t::readStop
 *
 * @retval  FSP_SUCCESS                  AT transaction aborted successfully.
 * @retval  FSP_ERR_ASSERTION            Pointer to AT control block is NULL.
 * @retval  FSP_ERR_NOT_OPEN             The control block has not been opened.
 * @return                       See @ref RENESAS_ERROR_CODES or functions called by this function for other possible
 *                               return codes. This function calls:
 *                                   * @ref transfer_api_t::disable
 **********************************************************************************************************************/
fsp_err_t RM_ATCMD_W_CORE_ReadStop (atcmd_w_ctrl_t * const p_at_ctrl, uint32_t * remaining_bytes)
{
    FSP_PARAMETER_NOT_USED(p_at_ctrl);
    FSP_PARAMETER_NOT_USED(remaining_bytes);

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * Sets the secure channel key for AT command core.
 *
 * This function copies the provided key into the internal control structure
 * if secure channel support is enabled (`ATCMD_SECURE_CHANNEL == 1`).
 *
 * @param[in] p_at_ctrl Pointer to the AT command control structure.
 * @param[in] key       Pointer to the secure key to be set.
 *
 * @retval FSP_SUCCESS  Key was set successfully (or no operation if secure channel is disabled).
 **********************************************************************************************************************/
fsp_err_t RM_ATCMD_W_CORE_SecureChannelKeySet (atcmd_w_ctrl_t * const p_at_ctrl, uint8_t * key)
{
    atcmd_w_core_instance_ctrl_t * p_ctrl = (atcmd_w_core_instance_ctrl_t *) p_at_ctrl;
    FSP_PARAMETER_NOT_USED(p_ctrl);
    FSP_PARAMETER_NOT_USED(key);
#if (ATCMD_SECURE_CHANNEL == 1)
    memcpy(p_ctrl->key, key, ATCMD_W_SECURE_KEY_MAX);
#endif

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @} (end addtogroup AT)
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/
#if (ATCMD_IF_SUPPORT == 1)
void rm_atcmd_w_core_init_rsp_evt_create (void)
{
    if (!init_rsp_evt_grp)
    {
        init_rsp_evt_grp = xEventGroupCreate();
    }
}

void rm_atcmd_w_core_init_rsp_evt_delete (void)
{
    if (init_rsp_evt_grp)
    {
        vEventGroupDelete(init_rsp_evt_grp);
        init_rsp_evt_grp = NULL;
    }
}

void rm_atcmd_w_core_init_rsp_evt_set (uint16_t event)
{
    if (init_rsp_evt_grp)
    {
        xEventGroupSetBits(init_rsp_evt_grp, event);
    }
}

fsp_err_t rm_atcmd_w_core_init_rsp_evt_wait (uint16_t event)
{
    EventBits_t events = xEventGroupWaitBits(init_rsp_evt_grp, event, pdFALSE, pdFALSE, portMAX_DELAY);

    if (events & ATCMD_W_CORE_EVT_WIFI_OPEN_DONE)
    {
        xEventGroupClearBits(init_rsp_evt_grp, ATCMD_W_CORE_EVT_WIFI_OPEN_DONE);

        events = xEventGroupWaitBits(init_rsp_evt_grp,
                                     ATCMD_W_CORE_EVT_TRANSPORT_INIT_DONE,
                                     pdTRUE,
                                     pdFALSE,
                                     portMAX_DELAY);
        if (events & ATCMD_W_CORE_EVT_TRANSPORT_INIT_DONE)
        {
            return FSP_SUCCESS;
        }
        else
        {
            return FSP_ERR_INVALID_STATE;
        }
    }
    else if (events & ATCMD_W_CORE_EVT_TRANSPORT_INIT_DONE)
    {
        xEventGroupClearBits(init_rsp_evt_grp, ATCMD_W_CORE_EVT_TRANSPORT_INIT_DONE);

        events = xEventGroupWaitBits(init_rsp_evt_grp, ATCMD_W_CORE_EVT_WIFI_OPEN_DONE, pdTRUE, pdFALSE, portMAX_DELAY);
        if (events & ATCMD_W_CORE_EVT_WIFI_OPEN_DONE)
        {
            return FSP_SUCCESS;
        }
        else
        {
            return FSP_ERR_INVALID_STATE;
        }
    }
    else if (events & ATCMD_W_CORE_EVT_INIT_RSP_SENT_DONE)
    {
        xEventGroupClearBits(init_rsp_evt_grp, ATCMD_W_CORE_EVT_INIT_RSP_SENT_DONE);

        return FSP_SUCCESS;
    }
    else
    {
        return FSP_ERR_INVALID_STATE;
    }
}

/* return : 0 (allow write), non-zero (not ready to send a response to mcu) */
static int rm_atcmd_w_core_wait_ready (atcmd_w_ctrl_t * const p_at_ctrl, uint32_t timeout_ms)
{
    uint32_t wait_count = timeout_ms / 10;

 #if CFG_PMGR
    if (!RM_PMGR_W_dpm_is_wakeup())
    {
        return 0;
    }
 #endif

    atcmd_w_core_instance_ctrl_t * p_ctrl = (atcmd_w_core_instance_ctrl_t *) p_at_ctrl;

    if (p_ctrl->init_rsp_not_sent)
    {
        return 0;
    }

 #if !CFG_PMGR
    p_ctrl->mcu_wu_done = pdTRUE;
 #endif

    while (1)
    {
        if (p_ctrl->mcu_wu_done == pdTRUE)
        {
            break;
        }

 #if CFG_PMGR
        if (RM_PMGR_W_dpm_sleep_is_started() == DONE_DPM_SLEEP)
        {
            return -1;
        }
 #endif                                // CFG_PMGR

        vTaskDelay(portCONVERT_MS_2_TICKS(10));

        if (--wait_count == 0)
        {
            return -1;
        }
    }

    return 0;
}

static fsp_err_atcmd_err_code rm_atcmd_w_core_at_parser (atcmd_w_ctrl_t * const       p_at_ctrl,
                                                         atcmd_w_core_module_list_t * p_list,
                                                         atcmd_w_uart_input_buff_t  * p_in)
{
    atcmd_w_core_instance_ctrl_t * p_ctrl = (atcmd_w_core_instance_ctrl_t *) p_at_ctrl;
    fsp_err_atcmd_err_code         err    = FSP_ERR_AT_CMD_ERR_CMD_OK;

    const atcmd_w_core_module_t      * p_module = NULL;
    const atcmd_w_core_module_node_t * p_module_node;
    char   * p_params[ATCMD_W_CORE_MAX_PARAMS] = {0x00, };
    uint32_t param_cnt      = 0;
    char   * p_cp_line      = NULL;
    int      is_exist_atcmd = false;
    char     ret_msg[20]    = {0x00, };

    if (p_in->at_cmd_req_idx == 0)
    {
        return FSP_ERR_AT_CMD_ERR_UNKNOWN_CMD;
    }

    p_cp_line = pvPortCalloc((p_in->at_cmd_req_idx + 2), sizeof(char));
    if (!p_cp_line)
    {
        return FSP_ERR_AT_CMD_ERR_MEM_ALLOC;
    }

    memcpy(p_cp_line, p_in->at_cmd_req_str, p_in->at_cmd_req_idx);
    p_cp_line[p_in->at_cmd_req_idx] = '\0';

 #if (ATCMD_SECURE_CHANNEL == 1)

    /* Following code is ugly hack - secure channel should be implemented as a separate layer */
    if (p_ctrl->secure_channel == 1)
    {
        /* Params must be encrypted when secure channel is enabled */
        char    * params = (char *) strchr(p_cp_line, '=');
        uint8_t   iv_local[AES_IV_SIZE_AT];
        int       len    = (int) p_in->sec_expected;
        uint8_t * cipher = NULL;
        int       avail  = 0;

        if (params != NULL)
        {
            params++;                  /* points to payload after '=' (ciphertext) */

            /* Plaintext exception: ATS=<32 hex IV> */
            if (memcmp(p_cp_line, "ATS=", 4) == 0)
            {
                if (is_valid_hex_str(params, IV_HEX_LEN_AT))
                {
                    /* Do NOT decrypt ATS=<IVHEX> */
                    goto SECCHAN_DONE;
                }
            }

            /* Only decrypt if RX framing armed and ciphertext was captured */
            if ((p_in->sec_active != 1) || (p_in->sec_expected == 0))
            {
                err = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
                goto end;
            }

            if ((p_in->sec_start_idx == 0) || (p_in->sec_start_idx >= p_in->at_cmd_req_idx))
            {
                err = FSP_ERR_AT_CMD_ERR_SECURE_RESYNC_REQ;
                goto end;
            }

            cipher = (uint8_t *) &p_cp_line[p_in->sec_start_idx];

            /* How many bytes are actually available after the cipher start */
            avail = (int) p_in->at_cmd_req_idx - (int) p_in->sec_start_idx;

            if ((len <= 0) || (len > avail))
            {
                err = FSP_ERR_AT_CMD_ERR_SECURE_RESYNC_REQ;
                goto end;
            }

            if ((len < AES_BLOCK_SIZE) || ((len % AES_BLOCK_SIZE) != 0))
            {
                err = FSP_ERR_AT_CMD_ERR_SECURE_RESYNC_REQ;
                goto end;
            }

            memcpy(iv_local, p_ctrl->rx_iv, AES_IV_SIZE_AT);

            int ret =
                mbedtls_aes_crypt_cbc(p_ctrl->ctx_rx,
                                      MBEDTLS_AES_DECRYPT,
                                      (size_t) len,
                                      iv_local,
                                      (const uint8_t *) cipher,
                                      cipher);

            if (ret != 0)
            {
                err = FSP_ERR_AT_CMD_ERR_SECURE_RESYNC_REQ;
                goto end;
            }

            int newlen = rm_atcmd_secchan_unpad((char *) cipher, len);

            if (newlen <= 0)
            {
                err = FSP_ERR_AT_CMD_ERR_SECURE_RESYNC_REQ;
                goto end;
            }

            memcpy(p_ctrl->rx_iv, iv_local, AES_IV_SIZE_AT);

            int max_plain = ATCMD_W_RESP_LEN_MAX - (int) p_in->at_cmd_req_idx + avail - 1;

            if (newlen > max_plain)
            {
                err = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
                goto end;
            }

            /* Copy plaintext to just after '=' */
            memmove(params, cipher, (size_t) newlen);
            params[newlen] = AT_CMD_END_OF_STR;
        }

SECCHAN_DONE:
        ;                              /* nothing */
    }
 #endif

    /* First call to strtok. */
    if (strchr(p_cp_line, '=') == NULL)
    {
        /* ' ' */
        p_params[param_cnt++] = strtok(p_cp_line, " ");
    }
    else
    {
        /* '=' */
        p_params[param_cnt++] = strtok(p_cp_line, AT_CMD_CLASS_BC_EXT);
    }

    /* No command entered */
    if (p_params[0] == NULL)
    {
        err = FSP_ERR_AT_CMD_ERR_UNKNOWN_CMD;
    }
    else
    {
        /* Find the AT-Command */
        for (p_module_node = p_list->p_module_head; p_module_node != NULL; p_module_node = p_module_node->next)
        {
            for (p_module = p_module_node->module; p_module->p_at_cmd != NULL; p_module++)
            {
                if (strcasecmp(p_module->p_at_cmd, p_params[0]) == 0)
                {
                    is_exist_atcmd = true;
                    break;
                }
            }

            if (is_exist_atcmd)
            {
                break;
            }
        }

        if (!is_exist_atcmd)
        {
            err = FSP_ERR_AT_CMD_ERR_UNKNOWN_CMD;
        }
        else
        {
 #if (ATCMD_SECURE_CHANNEL == 1)

            /* Check secure unfixed command first */
            /* It does not need to parse argument */
            if ((p_ctrl->secure_channel == 1) && (p_module->at_cmd_type == ATCMD_W_TYPE_SECURE_UNFIXED))
            {
                /* run command */
                if ((err == FSP_ERR_AT_CMD_ERR_CMD_OK) && (p_module->p_cmd_callback != NULL))
                {
                    p_params[1] = &p_cp_line[strlen(p_params[0]) + 1];
                    err         = p_module->p_cmd_callback(p_at_ctrl, 2, p_params);
                    goto end;
                }
            }
            else if ((p_ctrl->secure_channel == 0) && (p_module->at_cmd_type == ATCMD_W_TYPE_SECURE_UNFIXED))
            {
                is_exist_atcmd = false;
                err            = FSP_ERR_AT_CMD_ERR_UNKNOWN_CMD;
                goto end;              /* should not call the unfixed command in non-secure channel */
            }
 #endif

            /* To check if first character is ',' or not */
            /* command length + '=' */
            if (((p_cp_line[strlen(p_params[0])] == '=') || (p_cp_line[strlen(p_params[0])] == ' ')) &&
                (p_cp_line[strlen(p_params[0]) + 1] == ','))
            {
                err = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
                goto end;
            }

            if (strstr(p_cp_line, ",,") != NULL)
            {
                err = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
                goto end;
            }

            /* Parse arguments */
            while (((p_params[param_cnt] = strtok(NULL, AT_CMD_VAR_MRK)) != NULL))
            {
                if (p_params[param_cnt][0] == '\'')
                {
                    char * tmp_ptr = NULL;

                    /* Restore delimit-character */
                    p_params[param_cnt][strlen(p_params[param_cnt])] = ',';

                    if (strncmp(p_params[param_cnt], "',", 2) == 0)
                    {
                        /* First argument : AT+XXX=',aaaaa','bbbb' */
                        if (param_cnt == 1)
                        {
                            if ((tmp_ptr = strstr(&p_params[param_cnt][1], "',")) != NULL)
                            {
                                p_params[param_cnt] = p_params[param_cnt] + 1;
                                strtok(tmp_ptr, AT_CMD_VAR_MRK);
                                *tmp_ptr       = '\0';
                                *(tmp_ptr + 1) = '\0';
                            }
                        }
                        else
                        {
                            if (p_params[param_cnt][strlen(p_params[param_cnt]) - 1] == '\'')
                            {
                                tmp_ptr             = p_params[param_cnt] + strlen(p_params[param_cnt]) - 1;
                                p_params[param_cnt] = p_params[param_cnt] + 1;
                                strtok(tmp_ptr, "'");
                                *tmp_ptr = '\0';
                            }
                            else
                            {
                                err = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
                                goto end;
                            }
                        }
                    }
                    else if ((tmp_ptr = strstr(p_params[param_cnt], "',")) != NULL)
                    {
                        p_params[param_cnt] = p_params[param_cnt] + 1;
                        strtok(tmp_ptr, AT_CMD_VAR_MRK);
                        *tmp_ptr       = '\0';
                        *(tmp_ptr + 1) = '\0';
                    }
                    else if (p_params[param_cnt][strlen(p_params[param_cnt]) - 1] == '\'')
                    {
                        tmp_ptr             = p_params[param_cnt] + strlen(p_params[param_cnt]) - 1;
                        p_params[param_cnt] = p_params[param_cnt] + 1;
                        strtok(tmp_ptr, "'");
                        *tmp_ptr = '\0';
                    }
                    else
                    {
                        err = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
                        goto end;
                    }
                }
                else
                {
                    if (strstr(p_params[param_cnt], "'") != NULL)
                    {
                        err = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
                        goto end;
                    }
                }

                param_cnt++;

                if (param_cnt > (ATCMD_W_CORE_MAX_PARAMS - 1))
                {
                    err = FSP_ERR_AT_CMD_ERR_TOO_MANY_ARGS;
                    break;
                }
            }

            /* Check arguments */
            if (param_cnt - 1 > p_module->input_var)
            {
                if (strcasecmp(p_params[1], "?") && strcasecmp(p_params[1], "HELP"))
                {
                    /* param cnt exceeding check */
                    err = FSP_ERR_AT_CMD_ERR_TOO_MANY_ARGS;
                }
                else if ((p_module->input_var == 0) && (strcasecmp(p_params[1], "?") == 0))
                {
                    /* error on <cmd>=? where <cmd> has no p_params */
                    err = FSP_ERR_AT_CMD_ERR_TOO_MANY_ARGS;
                }
            }

            /* run command */
            if ((err == FSP_ERR_AT_CMD_ERR_CMD_OK) && (p_module->p_cmd_callback != NULL))
            {
                err = p_module->p_cmd_callback(p_at_ctrl, param_cnt, p_params);
            }
        }

end:

        if (err == FSP_ERR_AT_CMD_ERR_CMD_OK)
        {
            if ((strcasecmp(p_params[0], "ATB") != 0) && (strcasecmp(p_params[0], "AT+HOSTINITDONE") != 0))
            {
                if (p_ctrl->q_result == 1)
                {
                    strcpy(ret_msg, "\r\nOK\r\n");
 #if (ATCMD_TRANSPORT_SDIO_W == 1)
                    RM_ATCMD_W_CORE_Write(p_at_ctrl, (uint8_t const *) ret_msg, strlen(ret_msg));
 #else
                    RM_ATCMD_W_CORE_Write(p_at_ctrl, (uint8_t const *) ret_msg, strlen(ret_msg));
 #endif
                }

 #if CFG_WIFI
                RM_ATCMD_W_CORE_NETWORK_MQTT_RESP_Handle(p_at_ctrl);
 #endif                                /* CFG_WIFI */
            }
        }
        else if (err == FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT)
        {
            /* Not Printed */
        }
        else
        {
            if (p_ctrl->q_result == 1)
            {
                rm_atcmd_w_core_common_print_error_code(p_at_ctrl, err);
            }
        }
    }

    if (p_cp_line)
    {
        vPortFree(p_cp_line);
        p_cp_line = NULL;
    }

    return err;
}

atcmd_w_core_module_t ** rm_atcmd_get_user_atcmd_ptr (void)
{
    return &at_core_user_cmd_module;
}

atcmd_w_core_unfixed_module_t ** rm_atcmd_get_user_unfixed_atcmd_ptr (void)
{
    return &at_core_unfixed_user_cmd_module;
}

uint32_t RM_ATCMD_W_CORE_USER_CMD_open (atcmd_w_ctrl_t * const p_atcmd_w_ctrl)
{
    if (atcmd_open_user_app_callback != NULL)
    {
        atcmd_open_user_app_callback(p_atcmd_w_ctrl);
    }

    return FSP_ERR_AT_CMD_ERR_CMD_OK;
}

uint32_t RM_ATCMD_W_CORE_USER_CMD_close (atcmd_w_ctrl_t * const p_atcmd_w_ctrl)
{
    if (atcmd_close_user_app_callback != NULL)
    {
        atcmd_close_user_app_callback(p_atcmd_w_ctrl);
    }

    return FSP_ERR_AT_CMD_ERR_CMD_OK;
}

uint32_t RM_ATCMD_W_CORE_USER_CMD_register (atcmd_w_core_module_list_t * p_list)
{
 #if (ATCMD_W_CFG_PARAM_CHECKING_ENABLE)
    FSP_ASSERT(p_list);
 #endif

    if (NULL != at_core_user_cmd_module)
    {
        if (p_list->module_cnt >= ATCMD_W_LIST_MAX_CNT)
        {
            return FSP_ERR_AT_CMD_ERR_NOT_SUPPORTED;
        }

        if (rm_atcmd_w_core_register_module_node(p_list, at_core_user_cmd_module) == FSP_ERR_AT_CMD_ERR_MEM_ALLOC)
        {
            return FSP_ERR_AT_CMD_ERR_MEM_ALLOC;
        }
    }

    if (NULL != at_core_unfixed_user_cmd_module)
    {
        if (p_list->unfixed_module_cnt >= ATCMD_W_LIST_MAX_CNT)
        {
            return FSP_ERR_AT_CMD_ERR_NOT_SUPPORTED;
        }

        if (rm_atcmd_w_core_register_unfixed_module_node(p_list, at_core_unfixed_user_cmd_module) ==
            FSP_ERR_AT_CMD_ERR_MEM_ALLOC)
        {
            return FSP_ERR_AT_CMD_ERR_MEM_ALLOC;
        }
    }

    return FSP_ERR_AT_CMD_ERR_CMD_OK;
}

uint32_t RM_ATCMD_W_CORE_USER_CMD_deregister (atcmd_w_core_module_list_t * p_list)
{
 #if (ATCMD_W_CFG_PARAM_CHECKING_ENABLE)
    FSP_ASSERT(p_list);
 #endif
    if (NULL != at_core_user_cmd_module)
    {
        rm_atcmd_w_core_deregister(p_list, at_core_user_cmd_module);
    }

    if (NULL != at_core_unfixed_user_cmd_module)
    {
        rm_atcmd_w_core_unfixed_deregister(p_list, at_core_unfixed_user_cmd_module);
    }

    return FSP_ERR_AT_CMD_ERR_CMD_OK;
}

static fsp_err_atcmd_err_code rm_atcmd_w_core_common_parser (atcmd_w_ctrl_t * const       p_at_ctrl,
                                                             atcmd_w_core_module_list_t * p_list,
                                                             atcmd_w_uart_input_buff_t  * p_in)
{
 #if (ATCMD_DA14XXX_CODELESS == 1)

    /* All remote commands (ATr*) should be bypassed */
    if (!memcmp(p_in->at_cmd_req_str, "ATr", 3) && (sizeof(p_in->at_cmd_req_str) > p_in->at_cmd_req_idx))
    {
        p_in->at_cmd_req_str[p_in->at_cmd_req_idx] = '\r';

        return RM_ATCMD_W_CORE_DA14xxx_ProcRemoteCmd(p_at_ctrl, p_in->at_cmd_req_str, p_in->at_cmd_req_idx + 1);
    }
    else
 #endif
    if (p_in->at_cmd_req_idx == 0)
    {
        return FSP_ERR_AT_CMD_ERR_UNKNOWN_CMD;
    }

    return rm_atcmd_w_core_at_parser(p_at_ctrl, p_list, p_in);
}

static fsp_err_atcmd_err_code rm_atcmd_w_core_unfixed_parser (atcmd_w_ctrl_t * const       p_at_ctrl,
                                                              atcmd_w_core_module_list_t * p_list,
                                                              atcmd_w_uart_input_buff_t  * p_in)
{
    const atcmd_w_core_unfixed_module_t      * p_module = NULL;
    const atcmd_w_core_unfixed_module_node_t * p_module_node;

    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_UNKNOWN_CMD;

    if ((p_list->unfixed_module_cnt == 0) || (p_in->at_cmd_req_idx == 0))
    {
        return err;
    }

    /* Find the AT-Command */
    for (p_module_node = p_list->p_unfixed_module_head; p_module_node != NULL; p_module_node = p_module_node->next)
    {
        for (p_module = p_module_node->module; p_module->at_cmd_len != 0; p_module++)
        {
            if (((uint32_t) p_module->at_cmd_len == p_in->at_cmd_req_idx) &&
                (memcmp(p_module->at_cmd, (p_in->at_cmd_req_str), p_module->at_cmd_len) == 0))
            {
                err =
                    p_module->p_cmd_callback(p_at_ctrl, (uint8_t *) p_in->at_cmd_req_str, sizeof(p_in->at_cmd_req_str));

                goto end;
            }
        }
    }

end:

    return err;
}

uint32_t rm_atcmd_w_core_register_module_node (atcmd_w_core_module_list_t  * p_list,
                                               const atcmd_w_core_module_t * module)
{
    atcmd_w_core_module_node_t * new_node;

    new_node = pvPortMalloc(sizeof(atcmd_w_core_module_node_t));
    if (!new_node)
    {
        return FSP_ERR_AT_CMD_ERR_MEM_ALLOC;
    }

    new_node->module      = module;
    new_node->next        = p_list->p_module_head;
    p_list->p_module_head = new_node;
    p_list->module_cnt++;

    return FSP_ERR_AT_CMD_ERR_CMD_OK;
}

uint32_t rm_atcmd_w_core_register_unfixed_module_node (atcmd_w_core_module_list_t          * p_list,
                                                       const atcmd_w_core_unfixed_module_t * module)
{
    atcmd_w_core_unfixed_module_node_t * new_node;

    new_node = pvPortMalloc(sizeof(atcmd_w_core_unfixed_module_node_t));
    if (!new_node)
    {
        return FSP_ERR_AT_CMD_ERR_MEM_ALLOC;
    }

    new_node->module              = module;
    new_node->next                = p_list->p_unfixed_module_head;
    p_list->p_unfixed_module_head = new_node;
    p_list->unfixed_module_cnt++;

    return FSP_ERR_AT_CMD_ERR_CMD_OK;
}

void rm_atcmd_w_core_deregister (atcmd_w_core_module_list_t * p_list, const atcmd_w_core_module_t * module)
{
    atcmd_w_core_module_node_t * p_module_node;
    atcmd_w_core_module_node_t * p_module_prev = NULL;

    for (p_module_node = p_list->p_module_head; p_module_node != NULL;
         p_module_prev = p_module_node, p_module_node = p_module_node->next)
    {
        if (p_module_node->module == module)
        {
            if (p_module_node == p_list->p_module_head) // First node
            {
                p_list->p_module_head = p_list->p_module_head->next;
            }
            else
            {
                p_module_prev->next = p_module_node->next;
            }

            vPortFree(p_module_node);
            p_list->module_cnt--;
            break;
        }
    }
}

void rm_atcmd_w_core_unfixed_deregister (atcmd_w_core_module_list_t          * p_list,
                                         const atcmd_w_core_unfixed_module_t * module)
{
    atcmd_w_core_unfixed_module_node_t * p_module_node;
    atcmd_w_core_unfixed_module_node_t * p_module_prev = NULL;

    for (p_module_node = p_list->p_unfixed_module_head; p_module_node != NULL;
         p_module_prev = p_module_node, p_module_node = p_module_node->next)
    {
        if (p_module_node->module == module)
        {
            if (p_module_node == p_list->p_unfixed_module_head) // First node
            {
                p_list->p_unfixed_module_head = p_list->p_unfixed_module_head->next;
            }
            else
            {
                p_module_prev->next = p_module_node->next;
            }

            vPortFree(p_module_node);
            p_list->unfixed_module_cnt--;
            break;
        }
    }
}

 #if (ATCMD_SECURE_CHANNEL == 1)
static bool secchan_try_arm_len (atcmd_w_uart_input_buff_t * rx_data)
{
    if (rx_data->sec_active == 1)
    {
        return false;
    }

    /* Look for '=' then parse digits until ',' */
    char * eq    = (char *) strchr((char *) rx_data->at_cmd_req_str, '=');
    char * comma = (char *) strchr((char *) rx_data->at_cmd_req_str, ',');
    char * p     = NULL;
    int    len   = 0;

    if (!eq || !comma)
    {
        return false;
    }

    p = eq + 1;

    /* Must start with a digit */
    if ((*p < '0') || (*p > '9'))
    {
        return false;
    }

    while ((*p >= '0') && (*p <= '9'))
    {
        len = (len * 10) + (*p - '0');

        if (len > ATCMD_W_RESP_LEN_MAX)
        {
            return false;              /* bound */
        }

        p++;
    }

    if (*p != ',')
    {
        return false;
    }

    if ((len <= 0) || ((len % AES_BLOCK_SIZE) != 0))
    {
        return false;
    }

    uint16_t comma_off = (uint16_t) (p - (char *) rx_data->at_cmd_req_str);

    /* Arm binary capture: ciphertext starts AFTER the comma */
    rx_data->sec_expected  = (uint16_t) len;
    rx_data->sec_start_idx = (uint16_t) (comma_off + 1U);
    rx_data->sec_active    = 1;

    return true;
}

 #endif                                /* ATCMD_SECURE_CHANNEL */

static void rm_atcmd_w_core_parser_main (void * pvParameters)
{
    atcmd_w_ctrl_t * const         p_at_ctrl = (atcmd_w_ctrl_t * const) pvParameters;
    atcmd_w_core_instance_ctrl_t * p_ctrl    = (atcmd_w_core_instance_ctrl_t *) p_at_ctrl;

    fsp_err_t                 err       = FSP_SUCCESS;
    fsp_err_atcmd_err_code    atcmd_err = FSP_ERR_AT_CMD_ERR_CMD_OK;
    uint8_t                   ch        = 0;
    atcmd_w_uart_input_buff_t rx_data   = {0x00, };

    atcmd_w_core_module_list_t * p_list = &p_ctrl->list;

    /* Set default value */
    p_ctrl->q_result    = 1;
    p_ctrl->mcu_wu_done = DEF_MCU_WAKEUP_DONE;

    /* NOTE: It's temporray code. It must be removed after code cleanup */
    vTaskDelay(portCONVERT_MS_2_TICKS(100));

    /* Register AT-CMD */
    RM_ATCMD_W_CORE_BASIC_register(p_list);
 #if CFG_WIFI
    RM_ATCMD_W_CORE_WIFI_open(p_at_ctrl);
    RM_ATCMD_W_CORE_WIFI_register(p_list);
 #endif                                /* CFG_WIFI */
    rm_atcmd_w_core_init_rsp_evt_set(ATCMD_W_CORE_EVT_WIFI_OPEN_DONE);
    rm_atcmd_w_core_init_rsp_evt_wait(ATCMD_W_CORE_EVT_INIT_RSP_SENT_DONE);
    rm_atcmd_w_core_init_rsp_evt_delete();
 #if CFG_WIFI
    RM_ATCMD_W_CORE_NETWORK_register(p_list);

    RM_ATCMD_W_CORE_NETWORK_MQTT_open(p_at_ctrl);
    RM_ATCMD_W_CORE_NETWORK_MQTT_register(p_list);

    RM_ATCMD_W_CORE_SOCKET_init_start(p_at_ctrl);

  #if CFG_PMGR
    RM_ATCMD_W_CORE_WEBSOCKET_open(p_at_ctrl);
  #endif                               /* CFG_PMGR */
    RM_ATCMD_W_CORE_WEBSOCKET_register(p_list);

    RM_ATCMD_W_CORE_HTTP_open(p_at_ctrl);
    RM_ATCMD_W_CORE_HTTP_register(p_list);
  #if (ATCMD_RF_TEST_SUPPORT == 1)
    RM_ATCMD_W_CORE_RF_TEST_open(p_at_ctrl);
    RM_ATCMD_W_CORE_RF_TEST_register(p_list);
  #endif

  #if (SUPPORT_FSP_RM_FS_W == 1)
    RM_ATCMD_W_CORE_FS_register(p_list);
    RM_ATCMD_W_CORE_FS_open(p_at_ctrl);
  #endif

  #if (SUPPORT_FSP_RM_OTA_W == 1)
    RM_ATCMD_W_CORE_OTA_open(p_at_ctrl);
    RM_ATCMD_W_CORE_OTA_register(p_list);
  #endif

  #if CFG_PMGR
    RM_ATCMD_W_CORE_DPM_open(p_at_ctrl);
    RM_ATCMD_W_CORE_DPM_register(p_list);
  #endif                               /* CFG_PMGR */
 #endif                                /* CFG_WIFI */

    RM_ATCMD_W_CORE_PIN_PORT_register(p_list);
    RM_ATCMD_W_CORE_PIN_PORT_open(p_at_ctrl);

 #if (ATCMD_DA14XXX_CODELESS == 1)
    err = RM_ATCMD_W_CORE_DA14xxx_Open(p_at_ctrl);
    if (err == FSP_SUCCESS)
    {
        RM_ATCMD_W_CORE_DA14xxx_Register(p_list);
    }
 #endif

 #if (__SUPPORT_MATTER_IOT__)
    RM_ATCMD_W_CORE_MATTER_open(p_at_ctrl);
    RM_ATCMD_W_CORE_MATTER_register(p_list);
 #endif

 #if CFG_WIFI
    RM_ATCMD_W_CORE_PROD_TEST_register(p_list);
 #endif                                /* CFG_WIFI */

    RM_ATCMD_W_CORE_USER_CMD_open(p_at_ctrl);
    RM_ATCMD_W_CORE_USER_CMD_register(p_list);

 #if (ATCMD_DA14XXX_CODELESS == 1)
  #if (ATCMD_PMGR_SUPPORT_ENABLE == 1)

    /* Notify AT initialization completion - workaround required for DPM */
    BaseType_t sem_err = xSemaphoreGive(g_atcmd_init_semaphore);
    if (pdTRUE != sem_err)
    {
        goto exit;
    }
  #endif

    rm_atcmd_w_core_task_hdl = xTaskGetCurrentTaskHandle();
 #endif

 #if (ATCMD_BLE_BRG == 1)
    p_ctrl->core_task_is_running = true;
    while (p_ctrl->run_mode == AT_MODE_RUN)
 #else
    while (1)
 #endif
    {
        /* Process data from the host */
        err = RM_ATCMD_W_CORE_Read(p_at_ctrl, &ch, 1);

 #if (ATCMD_DA14XXX_CODELESS == 1)

        /* Binary mode event loop */
        if (RM_ATCMD_W_CORE_DA14xxx_IsBinaryMode(p_at_ctrl))
        {
            RM_ATCMD_W_CORE_DA14xxx_BinaryWrite(p_at_ctrl, (const uint8_t * const) &ch, sizeof(ch));

            continue;
        }
 #endif

        if (err == FSP_SUCCESS)
        {
            uint8_t uch = (uint8_t) ch;

            /* Check AT-Command length */
            if (rx_data.at_cmd_req_idx >= (ATCMD_W_RESP_LEN_MAX - 2))
            {
                if ((uch == AT_CMD_NEW_LINE_CHAR) || (uch == AT_CMD_LINE_FEED_CHAR))
                {
                    /* Input command too long. So, clear buffer */
                    memset(&rx_data, 0x00, sizeof(rx_data));
                    continue;
                }

                /* Pass the remaining data larger than the MAX size */
                continue;
            }

 #if (ATCMD_TRANSPORT_SDIO_W == 1)
            if ((uch == 0x00) || (uch == AT_CMD_LINE_FEED_CHAR))
 #else
            if (uch == 0x00)
 #endif
            {
 #if (ATCMD_SECURE_CHANNEL != 1)
                continue;
 #endif
            }

 #if (ATCMD_W_CORE_ECHO_EN)
            if (p_ctrl->echo_on == 1)
            {
                RM_ATCMD_W_CORE_Write(p_at_ctrl, (uint8_t const *) &uch, sizeof(uch));
            }
 #endif

            if (('\r' == ch) && (rx_data.at_cmd_req_idx == 0))
            {
                RM_ATCMD_W_CORE_DA14xxx_BinaryWrite(p_at_ctrl, (const uint8_t * const) &ch, sizeof(ch));
            }

 #if (ATCMD_SECURE_CHANNEL == 1)
            if (!((p_ctrl->secure_channel == 1) && (rx_data.sec_active == 1)) &&
                ((uch == AT_CMD_DEL_TXT_CHAR) || (uch == AT_CMD_BS_KEY_CHAR)))
 #else
            if ((uch == AT_CMD_DEL_TXT_CHAR) || (uch == AT_CMD_BS_KEY_CHAR))
 #endif                                /* ATCMD_SECURE_CHANNEL */
            {
                if (rx_data.at_cmd_req_idx > 0)
                {
                    rx_data.at_cmd_req_str[rx_data.at_cmd_req_idx] = AT_CMD_END_OF_STR;
                    rx_data.at_cmd_req_idx--;
                }
            }

 #if (ATCMD_TRANSPORT_SDIO_W == 1)
            else if (uch == AT_CMD_NEW_LINE_CHAR)
 #else
            else if ((uch == AT_CMD_NEW_LINE_CHAR) || (uch == AT_CMD_LINE_FEED_CHAR))
 #endif
            {
 #if (ATCMD_SECURE_CHANNEL == 1)

                /* ================= Secure-channel: ciphertext capture ================= */
                if ((p_ctrl->secure_channel == 1) && (rx_data.sec_active == 1))
                {
                    /* In LEN framing, CR/LF are valid ciphertext bytes */
                    if (rx_data.at_cmd_req_idx < (ATCMD_W_RESP_LEN_MAX - 1))
                    {
                        rx_data.at_cmd_req_str[rx_data.at_cmd_req_idx++] = (char) uch;
                    }
                    else
                    {
                        memset(&rx_data, 0x00, sizeof(rx_data));
                        continue;
                    }

                    uint16_t got = (uint16_t) (rx_data.at_cmd_req_idx - rx_data.sec_start_idx);

                    if (got < rx_data.sec_expected)
                    {
                        continue;
                    }

                    if (got > rx_data.sec_expected)
                    {
                        memset(&rx_data, 0x00, sizeof(rx_data));
                        continue;
                    }

                    rx_data.at_cmd_req_str[rx_data.at_cmd_req_idx] = AT_CMD_END_OF_STR;
                    atcmd_err = rm_atcmd_w_core_common_parser(p_at_ctrl, p_list, &rx_data);
                    memset(&rx_data, 0x00, sizeof(rx_data));
                    continue;
                }

                /* ================= Secure-channel: seen '=' but not armed yet ================= */
                if ((p_ctrl->secure_channel == 1) && (rx_data.sec_active == 0))
                {
                    if (rx_data.at_cmd_req_idx < (ATCMD_W_RESP_LEN_MAX - 1))
                    {
                        rx_data.at_cmd_req_str[rx_data.at_cmd_req_idx] = AT_CMD_END_OF_STR;
                    }

                    if (strchr((char *) rx_data.at_cmd_req_str, '=') != NULL)
                    {
                        if (secchan_try_arm_len(&rx_data))
                        {
                            continue;
                        }
                    }
                }
 #endif                                /* ATCMD_SECURE_CHANNEL */

                /* ================= Normal plaintext EOL ================= */
                if (rx_data.at_cmd_req_idx == 0)
                {
                    continue;
                }

                atcmd_err = rm_atcmd_w_core_common_parser(p_at_ctrl, p_list, &rx_data);
                memset(&rx_data, 0x00, sizeof(rx_data));
            }
            else
            {
                if (rx_data.at_cmd_req_idx < (ATCMD_W_RESP_LEN_MAX - 1))
                {
                    rx_data.at_cmd_req_str[rx_data.at_cmd_req_idx++] = (char) uch;
                }

 #if (ATCMD_SECURE_CHANNEL == 1)
                if (p_ctrl->secure_channel == 1)
                {
                    if (rx_data.sec_active == 1)
                    {
                        uint16_t got = (uint16_t) (rx_data.at_cmd_req_idx - rx_data.sec_start_idx);

                        if (got < rx_data.sec_expected)
                        {
                            continue;  /* still collecting */
                        }

                        if (got > rx_data.sec_expected)
                        {
                            memset(&rx_data, 0x00, sizeof(rx_data));
                            continue;
                        }

                        /* got == expected: exact completion */
                        if (rx_data.at_cmd_req_idx < (ATCMD_W_RESP_LEN_MAX - 1))
                        {
                            rx_data.at_cmd_req_str[rx_data.at_cmd_req_idx] = AT_CMD_END_OF_STR;
                        }

                        atcmd_err = rm_atcmd_w_core_common_parser(p_at_ctrl, p_list, &rx_data);
                        memset(&rx_data, 0x00, sizeof(rx_data));
                        continue;
                    }

                    if (rx_data.at_cmd_req_idx < (ATCMD_W_RESP_LEN_MAX - 1))
                    {
                        rx_data.at_cmd_req_str[rx_data.at_cmd_req_idx] = AT_CMD_END_OF_STR;
                    }

                    if (strchr((char *) rx_data.at_cmd_req_str, '=') != NULL)
                    {
                        (void) secchan_try_arm_len(&rx_data);
                        continue;
                    }
                }
 #endif                                /* ATCMD_SECURE_CHANNEL */

                /* Checked unfixed AT-CMD */
                atcmd_err = rm_atcmd_w_core_unfixed_parser(p_at_ctrl, p_list, &rx_data);
                if (atcmd_err != FSP_ERR_AT_CMD_ERR_UNKNOWN_CMD)
                {
                    /* Clear RX buffer */
                    memset(&rx_data, 0x00, sizeof(rx_data));
                }
            }
        }
    }

    /* Deregister AT-CMD */
    RM_ATCMD_W_CORE_BASIC_deregister(p_list);

 #if CFG_WIFI
    RM_ATCMD_W_CORE_WIFI_deregister(p_list);
    RM_ATCMD_W_CORE_WIFI_close(p_at_ctrl);
    RM_ATCMD_W_CORE_NETWORK_deregister(p_list);

    RM_ATCMD_W_CORE_NETWORK_MQTT_deregister(p_list);

    RM_ATCMD_W_CORE_SOCKET_deregister(p_list);
    RM_ATCMD_W_CORE_SOCKET_close(p_at_ctrl);

    RM_ATCMD_W_CORE_WEBSOCKET_deregister(p_list);
  #if CFG_PMGR
    RM_ATCMD_W_CORE_WEBSOCKET_close(p_at_ctrl);
  #endif                               /* CFG_PMGR */

    RM_ATCMD_W_CORE_HTTP_deregister(p_list);
    RM_ATCMD_W_CORE_HTTP_close(p_at_ctrl);

    RM_ATCMD_W_CORE_OTA_deregister(p_list);
    RM_ATCMD_W_CORE_OTA_close(p_at_ctrl);

  #if CFG_PMGR
    RM_ATCMD_W_CORE_DPM_deregister(p_list);
    RM_ATCMD_W_CORE_DPM_close(p_at_ctrl);
  #endif                               /* CFG_PMGR */
 #endif                                /* CFG_WIFI */

    RM_ATCMD_W_CORE_PIN_PORT_deregister(p_list);
    RM_ATCMD_W_CORE_PIN_PORT_close(p_at_ctrl);

    RM_ATCMD_W_CORE_PROD_TEST_deregister(p_list);

 #if (ATCMD_RF_TEST_SUPPORT == 1)
    RM_ATCMD_W_CORE_RF_TEST_deregister(p_list);
    RM_ATCMD_W_CORE_RF_TEST_close(p_at_ctrl);
 #endif

    RM_ATCMD_W_CORE_USER_CMD_deregister(p_list);
    RM_ATCMD_W_CORE_USER_CMD_close(p_at_ctrl);

 #if (SUPPORT_FSP_RM_FS_W == 1)
    RM_ATCMD_W_CORE_FS_deregister(p_list);
    RM_ATCMD_W_CORE_FS_close(p_at_ctrl);
 #endif

 #if (ATCMD_DA14XXX_CODELESS == 1)
    err = RM_ATCMD_W_CORE_DA14xxx_Unregister(p_list);
    if (err != FSP_SUCCESS)
    {
        goto exit;
    }

    err = RM_ATCMD_W_CORE_DA14xxx_Close(p_at_ctrl);
    if (err != FSP_SUCCESS)
    {
        goto exit;
    }
exit:
 #endif

 #if (ATCMD_BLE_BRG == 1)
    p_ctrl->core_task_is_running = false;
 #endif

    rm_atcmd_w_core_parser_task = NULL;

    vTaskDelete(NULL);
}

static fsp_err_t rm_atcmd_w_core_init_parser (atcmd_w_ctrl_t * const p_at_ctrl)
{
    xTaskCreate(rm_atcmd_w_core_parser_main, "AT-CORE-PARSER", (rm_atcmd_w_core_paser_task_size), (void *) p_at_ctrl,
                (OS_TASK_PRIORITY_LOWEST + ATCMD_W_MAIN_PARSER_PRIO), &rm_atcmd_w_core_parser_task);

    return FSP_SUCCESS;
}

#endif

#if (ATCMD_SECURE_CHANNEL == 1)

/* AES padding function (PKCS7) */
int rm_atcmd_secchan_pad (char * data, int length)
{
    int padding_size = 0, i;

    if (!data || (length < 0))
    {
        return -1;
    }

    padding_size = AES_BLOCK_SIZE - (length % AES_BLOCK_SIZE);
    padding_size = (padding_size == 0) ? AES_BLOCK_SIZE : padding_size;
    for (i = length; i < (length + padding_size); i++)
    {
        data[i] = (char) padding_size;
    }

    return length + padding_size;
}

int rm_atcmd_secchan_unpad (char * data, int length)
{
    uint8_t pad;

    if (!data || (length <= 0))
    {
        return -1;
    }

    pad = (uint8_t) data[length - 1];

    if ((pad == 0) || (pad > AES_BLOCK_SIZE) || (pad > length))
    {
        return -1;
    }

    data[length - pad] = '\0';

    return length - pad;
}

int is_hex_char (char c)
{
    return (c >= '0' && c <= '9') ||
           (c >= 'A' && c <= 'F') ||
           (c >= 'a' && c <= 'f');
}

#endif                                 /* ATCMD_SECURE_CHANNEL */
