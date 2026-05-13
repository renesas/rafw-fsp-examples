/***********************************************************************************************************************

* File Name    : red_ota_http_client.c

* Description  : Red OTA HTTP Client

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates

*

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/


#include "custom_config_sdk.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "mbedtls/ssl.h"
#include "rm_vee_flash_w_rrq_nvram.h"
#include "rm_map_persistant_w.h"
#include "rm_cert.h"
#include "rm_https_api.h"
#include "rm_https_w.h"
#include "core_json.h"
#include "sdk_ver.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/* Debug Log level(Information) for HTTP Client */
#undef ENABLE_HTTPC_DEBUG
#undef ENABLE_HTTPC_DEBUG_INFO
#undef ENABLE_HTTPC_DEBUG_ERR
#undef ENABLE_HTTPC_DEBUG_DUMP

#if defined (ENABLE_HTTPC_DEBUG)
  #define HTTPC_PRINTF            printf
#else
  #define HTTPC_PRINTF(...)            do {} while (0)
#endif

#if defined (ENABLE_HTTPC_DEBUG_INFO)
  #define HTTPC_DEBUG_INFO(fmt, ...)   HTTPC_PRINTF("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
  #define HTTPC_DEBUG_INFO(...)        do {} while (0)
#endif /* ENABLE_HTTPC_DEBUG_INFO */

#if defined (ENABLE_HTTPC_DEBUG_DUMP)
  #define HTTPC_DEBUG_DUMP(...)   HTTPC_PRINTF(__VA_ARGS__)
#else
  #define HTTPC_DEBUG_DUMP(...)        do {} while (0)
#endif /* ENABLE_HTTPC_DEBUG_INFO */

#if defined (ENABLE_HTTPC_DEBUG_ERR)
  #define HTTPC_DEBUG_ERR(fmt, ...)    HTTPC_PRINTF("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
  #define HTTPC_DEBUG_ERR(...)         do {} while (0)
#endif /* ENABLE_HTTPC_DEBUG_ERR */

#define HTTPC_MIN_INCOMING_LEN        (1024 * 1)
#define HTTPC_MAX_INCOMING_LEN        (1024 * 20)
#define HTTPC_DEF_INCOMING_LEN        (1024 * 4)
#define HTTPC_MIN_OUTGOING_LEN        (1024 * 1)
#define HTTPC_MAX_OUTGOING_LEN        (1024 * 20)
#define HTTPC_DEF_OUTGOING_LEN        (1024 * 4)

#define HTTPC_MAX_ALPN_CNT            3
#define HTTPC_MAX_ALPN_LEN            24
#define HTTPC_MAX_SNI_LEN             64
#define HTTPC_MAX_STOP_TIMEOUT        (300 * HTTPC_DEF_TIMEOUT)

// NVRAM name of HTTP-CLIENT TLS version
#define HTTPC_NVRAM_CONFIG_TLS_VER       "HTTPC_TLS_VER"
// NVRAM name of HTTP-CLIENT TLS auth_mode
#define HTTPC_NVRAM_CONFIG_TLS_AUTH       "HTTPC_TLS_AUTHMODE"
// NVRAM name of HTTP-CLIENT TLS alpn
#define HTTPC_NVRAM_CONFIG_TLS_ALPN       "HTTPC_TLS_ALPN"
// NVRAM name of the number of TLS alpn
#define HTTPC_NVRAM_CONFIG_TLS_ALPN_NUM   "HTTPC_TLS_ALPN_NUM"
// NVRAM name of HTTP-CLIENT TLS SNI
#define HTTPC_NVRAM_CONFIG_TLS_SNI        "HTTPC_TLS_SNI"

/***********************************************************************************************************************
 * Function prototypes
 ***********************************************************************************************************************/
err_t red_ota_run_http_client(char *url, char *sdk_version, char *image_name,
    char *mcu_fw_update, char *mcu_fw_image_name,  char *cert_key_update, char *cert_key_image_name);
void red_ota_httpc_recv_callback(https_callback_args_t * p_args);

/***********************************************************************************************************************
 * Exported global variables (to be accessed by other files)
 **********************************************************************************************************************/
extern const https_api_t g_https_w;
const https_api_t * const p_red_ota_https = &g_https_w;

https_w_instance_ctrl_t g_red_ota_https_w0_ctrl;

https_cfg_t g_red_ota_https_w0_cfg =
{
    .p_callback    = red_ota_httpc_recv_callback,
};

static volatile uint32_t client_transfer_complete               = 0;
static volatile uint32_t client_transfer_error                  = 0;
static volatile uint32_t server_transfer_complete               = 0;
static volatile uint32_t server_transfer_error                  = 0;

static http_client_request_t red_ota_request = {0, };

char *g_ota_sdk_version = NULL;
char *g_ota_image_name = NULL;
char *g_mcu_fw_update = NULL;
char *g_mcu_fw_image_name = NULL;
char *g_cert_key_update = NULL;
char *g_cert_key_image_name = NULL;

volatile int ota_read_flag;
static unsigned int is_ota_read(void)
{
    return ota_read_flag;
}

static void http_client_clear_alpn(httpc_secure_connection_t *conf)
{
    if (conf->alpn)
    {
        for (int idx = 0 ; idx < conf->alpn_cnt ; idx++)
        {
            if (conf->alpn[idx])
            {
                vPortFree(conf->alpn[idx]);
                conf->alpn[idx] = NULL;
            }
        }
        vPortFree(conf->alpn);
        conf->alpn = NULL;
        conf->alpn_cnt = 0;
    }

    return ;
}

static void http_client_clear_https_conf(httpc_secure_connection_t *conf)
{

    if (conf)
    {
        if (conf->ca_len > 0)
        {
            vPortFree(conf->ca);
            conf->ca = NULL;
            conf->ca_len = 0;
        }

        if (conf->cert_len > 0)
        {
            vPortFree(conf->cert);
            conf->cert = NULL;
            conf->cert_len = 0;
        }

        if (conf->privkey_len > 0)
        {
            vPortFree(conf->privkey);
            conf->privkey = NULL;
            conf->privkey_len = 0;
        }

        if (conf->dh_param_len > 0)
        {
            vPortFree(conf->dh_param);
            conf->dh_param = NULL;
            conf->dh_param_len = 0;
        }

        if (conf->sni_len > 0)
        {
            vPortFree(conf->sni);
            conf->sni = NULL;
            conf->sni_len = 0;
        }

        http_client_clear_alpn(conf);
    }

    return;
}

static JSONStatus_t ota_json_query_parse(const char *query, char *result_string,
                                         char *buffer, size_t bufferLength)
{
    JSONStatus_t result;
    char *  value;
    size_t valueLength;

    result = JSON_Search(buffer, bufferLength, query, strlen(query),
                            &value, &valueLength);

    if(result == JSONSuccess)
    {
        // The pointer "value" will point to a location in the "buffer".
        char save = value[valueLength];
        // After saving the character, set it to a null byte for printing.
        value[valueLength] = '\0';
        // "Found: bar.foo -> xyz" will be printed.
        HTTPC_DEBUG_INFO("Found: %s -> %s\n", query, value);
        if(result_string)
        {
            strcpy(result_string, value);
        }
        // Restore the original character.
        value[valueLength] = save;
    }
    else
    {
        HTTPC_DEBUG_ERR("JSON_Search queer %s error, result %d\n", query, result);
        return result;
    }

    return result;
}
static JSONStatus_t ota_json_parse(char *buffer, size_t bufferLength)
{
    JSONStatus_t result;

    // Calling JSON_Validate() is not necessary if the document is guaranteed to be valid.
    result = JSON_Validate(buffer, bufferLength);
    if(result != JSONSuccess)
    {
        HTTPC_DEBUG_ERR("JSON_Validate error, result %d\n", result);
        return result;
    }

    result = ota_json_query_parse("RTOS.version", g_ota_sdk_version, buffer, bufferLength);
    if(result != JSONSuccess)
    {
        return result;
    }
    result = ota_json_query_parse("RTOS.image", g_ota_image_name, buffer, bufferLength);
    if(result != JSONSuccess)
    {
        return result;
    }
    result = ota_json_query_parse("MCU_FW.update", g_mcu_fw_update, buffer, bufferLength);
    if(result != JSONSuccess)
    {
        return result;
    }
    result = ota_json_query_parse("MCU_FW.image", g_mcu_fw_image_name, buffer, bufferLength);
    if(result != JSONSuccess)
    {
        return result;
    }
    result = ota_json_query_parse("CERT_KEY.update", g_cert_key_update, buffer, bufferLength);
    if(result != JSONSuccess)
        {
        return result;
        }
    result = ota_json_query_parse("CERT_KEY.image", g_cert_key_image_name, buffer, bufferLength);
    if(result != JSONSuccess)
    {
        return result;
    }
    return result;
}

static err_t httpc_ota_response(char *payload, int len,
                                u32_t content_len, httpc_result_t httpc_result,
                                httpc_state_t *connection)
{
    RA6W1_UNUSED_ARG(content_len);
    RA6W1_UNUSED_ARG(httpc_result);
    RA6W1_UNUSED_ARG(connection);

    if(ota_json_parse(payload, len) != JSONSuccess)
    {
        HTTPC_DEBUG_ERR("ota.json parse error\n");
        return ERR_VAL;
    }

    return ERR_OK;
}

#if defined(ENABLE_HTTPC_DEBUG_INFO)
static void print_payload(char * payload, int len)
{
    char * buffer = NULL;

    buffer = pvPortMalloc(len);
    if (buffer == NULL)
    {
        HTTPC_DEBUG_ERR("Failed to allocate memory for at-cmd (need=%dbyte)\n", len);
    }

    memcpy(buffer, payload, (size_t) len);

    printf("HTTPC response %s\n", buffer);

    if (buffer != NULL)
    {
        vPortFree(buffer);
        buffer = NULL;
    }
}
#endif

static err_t check_auth_error(char *payload)
{
    err_t err = ERR_OK;
    if (strstr(payload, "401 Unauthorized") != NULL)
    {
        err = ERR_ARG;
    }

    return err;
}

void red_ota_httpc_recv_callback(https_callback_args_t *p_args)
{
    https_callback_args_t *p = (https_callback_args_t *)p_args;

    switch(p_args->event)
    {
        case HTTPS_EVENT_SERVER_RECVED:
        {
            err_t err = *(int8_t *) p_args->p_param;

            HTTPC_DEBUG_INFO("Event: HTTPS_EVENT_SERVER_RECVED\n");
            if (err != ERR_OK || !p_args->len)
            {
                HTTPC_DEBUG_ERR("[%s]:%d err=%d, p->len = %d\n", __func__, __LINE__, err, p_args->len);
                server_transfer_error = 1;
            }

            if (err == ERR_OK)
            {
                server_transfer_complete = 1;
            }

            HTTPC_DEBUG_INFO("\n[%s]:%d err = %d, p->len = %d\n",__func__, __LINE__, err, p_args->len);

            break;
        }

        case HTTPS_EVENT_SERVER_ERR_RESULT:
        {
            err_t err = *(int8_t *) p_args->p_param;

            HTTPC_DEBUG_INFO("Event: HTTPS_EVENT_SERVER_ERR_RESULT\n");
            HTTPC_DEBUG_ERR("err: %d\n", err);

            if(err != ERR_OK)
            {
                server_transfer_error = 1;
            }
            break;
        }

        case HTTPS_EVENT_CLIENT_RESULT:
        {
#if defined(ENABLE_HTTPC_DEBUG_INFO)
            uint32_t httpc_result = *(uint32_t *)p_args->payload;
            uint32_t rx_content_len = p_args->len;
#endif
            err_t err = *(int8_t *) p_args->p_param;

            if (err != 0)
            {
                ota_read_flag = err;
            }

            HTTPC_DEBUG_INFO("Event: HTTPS_EVENT_CLIENT_RESULT\n");
            HTTPC_DEBUG_INFO("httpc_result: %ld, received: %d byte, err: %d\n", httpc_result, (int)rx_content_len, err);

            if(err == ERR_OK)
            {
                client_transfer_complete = 1;
            }
            else
            {
                client_transfer_error = 1;
            }

            http_client_clear_https_conf(&red_ota_request.https_conf);
            p_red_ota_https->close(&g_red_ota_https_w0_ctrl);

            break;
        }

        case HTTPS_EVENT_CLIENT_RECVED:
        {
#if defined(ENABLE_HTTPC_DEBUG_INFO)
            uint32_t rx_content_len = p_args->len;
#endif
            err_t err = *(int8_t *) p_args->p_param;

            HTTPC_DEBUG_INFO("Event: HTTPS_EVENT_CLIENT_RECVED\n");
            HTTPC_DEBUG_INFO("received: %d byte, err: %d\n", (int)rx_content_len, err);

#if defined(ENABLE_HTTPC_DEBUG_INFO)
            print_payload((char *) p->payload, (int) p->len);
#endif
            if (!err)
            {
            	err = check_auth_error((char *) p->payload);
            }

            if (!err)
            {
                err = httpc_ota_response((char *) p->payload, (int)p->len, 0, 0, NULL);
            }

            ota_read_flag = err;

            break;
        }

        case HTTPS_EVENT_CLIENT_RECVED_DECODED:
        {
            HTTPC_DEBUG_INFO("Event: HTTPS_EVENT_CLIENT_RECVED_DECODED\n");
            break;
        }

        case HTTPS_EVENT_CLIENT_GET_DONE:
        {
            HTTPC_DEBUG_INFO("Event: HTTPS_EVENT_CLIENT_GET_DONE\n");
            break;
        }

        default:
            break;
    }
}

static fsp_err_t rm_ota_w_http_client_parse_request(const char *url, http_client_request_t *request)
{
    err_t err = ERR_OK;
    char *p = NULL;
    char *q = NULL;
    int len = 0;

    request->op_code = HTTP_CLIENT_OPCODE_READY;

    if (strncmp(url, "https", strlen("https")) == 0)
    {
        //https
        request->insecure = pdTRUE;
        request->port = HTTPS_SERVER_PORT;

        //http_client_clear_https_conf(&request->https_conf);

    }
    else if (strncmp(url, "http", strlen("http")) == 0)
    {
        //http
        request->insecure = pdFALSE;
        request->port = HTTP_SERVER_PORT;

    }
    else
    {
        err = ERR_ARG;
        goto exit;
    }

    memset(request->path, 0x00, HTTPC_MAX_PATH_LEN);
    memcpy(request->path, url, strlen(url));

    p = (char *)request->path;
    len = strlen((char *)request->path);

    q = strstr(p, "://");
    q += (strlen("://"));
    len -= q-p;
    p = q;

    q = strstr(p, "@");
    if (q != NULL)
    {
        q += (strlen("@"));
        len -= q-p;
        p = q;
    }

    q = strstr(p, ":");
    if (q != NULL)
    {
        /* IPv4 address or FQDN */
        memset(request->hostname, 0x00, HTTPC_MAX_HOSTNAME_LEN);
        memcpy(request->hostname, p, (size_t) (q - p));

        q += (strlen(":"));
        len -= q - p;
        p = q;

        while (len && isdigit((int)(*q)))
        {
            ++q;
            --len;
        }

        if (p < q)
        {    /* explicit port number given */
            int port = 0;

            while (p < q)
            {
                port = port * 10 + (*p++ - '0');
            }

            request->port = (UINT) port;
        }
    }
    else
    {
        q = strstr(p, "/");
        if ((q == NULL) && len)
        {
            err = ERR_ARG;
            goto exit;
        }
        if (strstr(p, "["))
        {
            /* IPv6 address reference */
            //not supported ipv6
            HTTPC_DEBUG_ERR("Not supported IPv6\n");
            err = ERR_ARG;
            goto exit;
        }
        /* IPv4 address or FQDN */
        memset(request->hostname, 0x00, HTTPC_MAX_HOSTNAME_LEN);
        memcpy(request->hostname, p, (size_t) (q - p));
    }

    request->op_code = HTTP_CLIENT_OPCODE_GET;

exit:
    return err;
}

static int http_client_read_cert(int module, int type, unsigned char **out, size_t *outlen)
{
    int ret = 0;
    unsigned char * buf = NULL;
    size_t buflen = RM_CERT_MAX_LENGTH;
    rm_cert_format_t format = RM_CERT_FORMAT_NONE;

    buf = pvPortMalloc(buflen);
    if (!buf)
    {
        HTTPC_DEBUG_ERR("Failed to allocate memory(module:%d, type:%d, len:%d)\r\n", module, type, (int)buflen);
        return -1;
    }

    memset(buf, 0x00, buflen);

    ret = RM_CERT_Read(module, type, &format, buf, &buflen);
    if (ret == RM_CERT_ERR_OK)
    {
        *out = buf;
        *outlen = buflen;
        return 0;
    }
    else if (ret == RM_CERT_ERR_EMPTY_CERTIFICATE)
    {
        if (buf)
        {
            vPortFree(buf);
            buf = NULL;
            *out = NULL;
            *outlen = 0;
        }
        return 0;
    }

    if (buf)
    {
        vPortFree(buf);
        buf = NULL;
        *out = NULL;
        *outlen = 0;
    }

    return -1;
}

static void http_client_read_certs(httpc_secure_connection_t *conf)
{
    int ret = 0;

    //to read ca certificate
    ret = http_client_read_cert(RM_CERT_MODULE_HTTPS_CLIENT, RM_CERT_TYPE_CA_CERT, &conf->ca, &conf->ca_len);
    if (ret)
    {
        HTTPC_DEBUG_ERR("failed to read CA cert\r\n");
        goto err;
    }
    if (conf->ca_len > 0)
    {
        HTTPC_DEBUG_INFO("Read CA(length = %d)\n", conf->ca_len);
    }

    //to read certificate
    ret = http_client_read_cert(RM_CERT_MODULE_HTTPS_CLIENT, RM_CERT_TYPE_CERT, &conf->cert, &conf->cert_len);
    if (ret)
    {
        HTTPC_DEBUG_ERR("failed to read certificate\r\n");
        goto err;
    }
    if (conf->cert_len > 0)
    {
        HTTPC_DEBUG_INFO("Read Cert(length = %d)\n", conf->cert_len);
    }

    //to read private key
    ret = http_client_read_cert(RM_CERT_MODULE_HTTPS_CLIENT, RM_CERT_TYPE_PRIVATE_KEY, &conf->privkey, &conf->privkey_len);
    if (ret)
    {
        HTTPC_DEBUG_ERR("failed to read private key\r\n");
        goto err;
    }
    if (conf->privkey_len > 0)
    {
        HTTPC_DEBUG_INFO("Read Privkey(length = %d)\n", conf->privkey_len);
    }

    //to read dh param
    ret = http_client_read_cert(RM_CERT_MODULE_HTTPS_CLIENT, RM_CERT_TYPE_DH_PARAMS, &conf->dh_param, &conf->dh_param_len);
    if (ret)
    {
        HTTPC_DEBUG_ERR("failed to read dh param\r\n");
        goto err;
    }
    if (conf->dh_param_len > 0)
    {
        HTTPC_DEBUG_INFO("Read DH Param(length = %d)\n", conf->dh_param_len);
    }

    return;

err:

    if (conf->ca)
    {
        vPortFree(conf->ca);
        conf->ca = NULL;
        conf->ca_len = 0;
    }

    if (conf->cert)
    {
        vPortFree(conf->cert);
        conf->cert = NULL;
        conf->cert_len = 0;
    }

    if (conf->privkey)
    {
        vPortFree(conf->privkey);
        conf->privkey = NULL;
        conf->privkey_len = 0;
    }

    if (conf->dh_param)
    {
        vPortFree(conf->dh_param);
        conf->dh_param = NULL;
        conf->dh_param_len = 0;
    }

    return ;
}

static fsp_err_t http_client_set_https_conf(http_client_request_t *request)
{
    fsp_err_t err = FSP_SUCCESS;
    int index = 0;
    int alpn_cnt = 0;
    int auth_mode = 0;
    int tls_ver = 0;
    char *sni_str = NULL;
    unsigned int sni_len = 0;

    //For tls
    if (request->insecure == pdTRUE)
    {
        request->https_conf.incoming_len = HTTPC_MAX_INCOMING_LEN;
        request->https_conf.outgoing_len = HTTPC_MAX_OUTGOING_LEN;

        //auth_mode
        RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, HTTPC_NVRAM_CONFIG_TLS_AUTH, &auth_mode);
        if (auth_mode != -1)
        {
            if ((auth_mode >= MBEDTLS_SSL_VERIFY_NONE) && (auth_mode < MBEDTLS_SSL_VERIFY_UNSET))
                request->https_conf.auth_mode = (u32_t)auth_mode;
            else
                request->https_conf.auth_mode = MBEDTLS_SSL_VERIFY_NONE;
        }
        else
        {
            request->https_conf.auth_mode = MBEDTLS_SSL_VERIFY_NONE;
        }

        //tls_ver
        RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, HTTPC_NVRAM_CONFIG_TLS_VER, &tls_ver);
        if (tls_ver != -1)
        {
            if (tls_ver == 0)
            {
                //ONLY_TLS12
                request->https_conf.tls_ver_min = MBEDTLS_SSL_VERSION_TLS1_2;
                request->https_conf.tls_ver_max = MBEDTLS_SSL_VERSION_TLS1_2;
            }
            else if (tls_ver == 1)
            {
                //ONLY_TLS13;
                request->https_conf.tls_ver_min = MBEDTLS_SSL_VERSION_TLS1_3;
                request->https_conf.tls_ver_max = MBEDTLS_SSL_VERSION_TLS1_3;
            }
            else if (tls_ver == 2)
            {
                //TLS12_13;
                request->https_conf.tls_ver_min = MBEDTLS_SSL_VERSION_TLS1_2;
                request->https_conf.tls_ver_max = MBEDTLS_SSL_VERSION_TLS1_3;
            }
            else
            {
                //ONLY_TLS12
                request->https_conf.tls_ver_min = MBEDTLS_SSL_VERSION_TLS1_2;
                request->https_conf.tls_ver_max = MBEDTLS_SSL_VERSION_TLS1_2;
            }
        }
        else
        {
            //ONLY_TLS12
            request->https_conf.tls_ver_min = MBEDTLS_SSL_VERSION_TLS1_2;
            request->https_conf.tls_ver_max = MBEDTLS_SSL_VERSION_TLS1_2;
        }

        //Read certificate
        http_client_read_certs(&request->https_conf);

        //sni
        RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, HTTPC_NVRAM_CONFIG_TLS_SNI, &sni_str);
        if (sni_str != NULL)
        {
            sni_len = strlen(sni_str);

            if ((sni_len > 0) && (sni_len < HTTPC_MAX_SNI_LEN))
            {
                if (request->https_conf.sni != NULL)
                {
                    vPortFree(request->https_conf.sni);
                    request->https_conf.sni = NULL;
                }

                request->https_conf.sni = pvPortMalloc(sni_len + 1);
                if (request->https_conf.sni == NULL)
                {
                    HTTPC_DEBUG_ERR("Failed to allocate SNI(%d)\n", sni_len);
                    err = FSP_ERR_OUT_OF_MEMORY;
                    goto exit;
                }

                strcpy(request->https_conf.sni, sni_str);
                request->https_conf.sni_len = (int)(sni_len + 1);

                HTTPC_DEBUG_INFO("ReadNVRAM SNI = %s\n", request->https_conf.sni);
            }
        }

        //alpn
        RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, HTTPC_NVRAM_CONFIG_TLS_ALPN_NUM, &alpn_cnt);
        if (alpn_cnt != -1)
        {
            if (alpn_cnt > 0)
            {
                http_client_clear_alpn(&request->https_conf);

                request->https_conf.alpn = pvPortMalloc((size_t)(alpn_cnt + 1) * sizeof(char *));
                if (!request->https_conf.alpn)
                {
                    HTTPC_DEBUG_ERR("Failed to allocate ALPN\n");
                    err = FSP_ERR_OUT_OF_MEMORY;
                    goto exit;
                }

                for (index = 0 ; index < alpn_cnt ; index++)
                {
                    char nvrName[16] = {0, };
                    char *alpn_str = NULL;

                    if (index >= HTTPC_MAX_ALPN_CNT)
                    {
                        break;
                    }

                    sprintf(nvrName, "%s%d", HTTPC_NVRAM_CONFIG_TLS_ALPN, index);
                    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, nvrName, &alpn_str);

            if(alpn_str != NULL)
            {
                        request->https_conf.alpn[index] = pvPortMalloc(strlen(alpn_str)+1);
            }
            else
            {
                HTTPC_DEBUG_ERR("alpn_str = NULL");
                err = FSP_ERR_OUT_OF_MEMORY;
                goto exit;
            }

                    if (!request->https_conf.alpn[index])
                    {
                        HTTPC_DEBUG_ERR("Failed to allocate ALPN#%d(len=%d)\n", index + 1, strlen(alpn_str));
                        http_client_clear_alpn(&request->https_conf);
                        err = FSP_ERR_OUT_OF_MEMORY;
                        goto exit;
                    }

                    request->https_conf.alpn_cnt = index + 1;
                    strcpy(request->https_conf.alpn[index], alpn_str);
                    HTTPC_DEBUG_INFO("ReadNVRAM ALPN#%d = %s\n",
                            request->https_conf.alpn_cnt,
                            request->https_conf.alpn[index]);
                }

                request->https_conf.alpn[index] = NULL;
            }
        }
    }
exit:
    if (err != FSP_SUCCESS)
    {
        http_client_clear_alpn(&request->https_conf);
    }
    return err;
}

err_t red_ota_run_http_client(char *url, char *sdk_version, char *image_name,
    char *mcu_fw_update, char *mcu_fw_image_name,  char *cert_key_update, char *cert_key_image_name)
{
    fsp_err_t err = FSP_SUCCESS;

    ota_read_flag = ERR_INPROGRESS;

    g_ota_sdk_version = sdk_version;
    g_ota_image_name = image_name;
    g_mcu_fw_update = mcu_fw_update;
    g_mcu_fw_image_name = mcu_fw_image_name;
    g_cert_key_image_name = cert_key_image_name;
    g_cert_key_update = cert_key_update;

    err = p_red_ota_https->open(&g_red_ota_https_w0_ctrl, &g_red_ota_https_w0_cfg);
    if (err == FSP_SUCCESS)
    {
        err = rm_ota_w_http_client_parse_request(url, &red_ota_request);
        if (err == FSP_SUCCESS)
        {
            http_client_set_https_conf(&red_ota_request);

            err = p_red_ota_https->clientSendRequest(&g_red_ota_https_w0_ctrl, &red_ota_request);
            if (err != FSP_SUCCESS)
            {
                HTTPC_DEBUG_ERR("clientSendRequest failed(err = %d)\n", err);
            }
        }
    }

    /* Waiting for read ota.json */
    while ((err = is_ota_read()) == (fsp_err_t)ERR_INPROGRESS)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    HTTPC_DEBUG_INFO("red_ota_run_http_client ret %d\n", err);
    return err;
}

/* EOF */
