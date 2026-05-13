/***********************************************************************************************************************
* File Name    : http_srv.c
* Description  : http(s) server functions and configurations
**********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "common_data.h"
#include "common_utils.h"
#include "http_svr.h"
#include "lfs.h"
#include "http_svr.h"
#include "rm_httpd.h"

https_server_sec_t p_sec;

static const uint8_t tls_srv_cert[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB6DCCAY2gAwIBAgIUPUuczXvddLrhm/ckOgE2CUa9bncwCgYIKoZIzj0EAwIw\n"
    "YTELMAkGA1UEBhMCSU4xDzANBgNVBAgMBktlcmFsYTESMBAGA1UEBwwJS296aGlr\n"
    "b2RlMRQwEgYDVQQKDAtFeGFtcGxlIElvVDEXMBUGA1UEAwwObXlkZXZpY2UubG9j\n"
    "YWwwHhcNMjUwNjE2MTAxOTI3WhcNMjYwNjE2MTAxOTI3WjBhMQswCQYDVQQGEwJJ\n"
    "TjEPMA0GA1UECAwGS2VyYWxhMRIwEAYDVQQHDAlLb3poaWtvZGUxFDASBgNVBAoM\n"
    "C0V4YW1wbGUgSW9UMRcwFQYDVQQDDA5teWRldmljZS5sb2NhbDBZMBMGByqGSM49\n"
    "AgEGCCqGSM49AwEHA0IABCZu8oIidrVJjASgssa5oavfCkQUI93zRxKKTuXN8tsW\n"
    "Aq9Upn8jcCSnZAGhWZhlEHCeyQN6cZFttwWNht56FX2jIzAhMB8GA1UdEQQYMBaC\n"
    "Dm15ZGV2aWNlLmxvY2FshwTAqDKcMAoGCCqGSM49BAMCA0kAMEYCIQDq01321W0O\n"
    "5BeDCx7+Ww/8fzQg6bW5bpI9mTbG99tSTwIhANYIrykIdtDU20unlS01j3cHQM+W\n"
    "ZMx/MmdF3yKUgctH\n"
    "-----END CERTIFICATE-----\n";

static size_t tls_srv_cert_len = sizeof(tls_srv_cert);

static const uint8_t tls_srv_key[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEINGUbjFG3GfiVsV29FG/rh8qjc3PkzqCf6B8CA/3310noAoGCCqGSM49\n"
    "AwEHoUQDQgAEJm7ygiJ2tUmMBKCyxrmhq98KRBQj3fNHEopO5c3y2xYCr1SmfyNw\n"
    "JKdkAaFZmGUQcJ7JA3pxkW23BY2G3noVfQ==\n"
    "-----END EC PRIVATE KEY-----\n";

static size_t tls_srv_key_len = sizeof(tls_srv_key);

void g_https0_callback(https_callback_args_t  *p_args)
{
    switch(p_args->event)
    {
        case HTTPS_EVENT_SERVER_RECVED:
        {
            APP_PRINT_INFO("Event: HTTPS_EVENT_SERVER_RECVED\n");
            break;
        }

        default:
            APP_PRINT_ERR("Error: Unknown event from http Server received\n");
    }
}

static void config_secure_connection(https_server_sec_t *p_sec_cfg)
{
    p_sec_cfg->p_tls_srv_cert = (uint8_t *)tls_srv_cert;
    p_sec_cfg->p_tls_srv_key = (uint8_t *)tls_srv_key;
    p_sec_cfg->tls_srv_cert_len = tls_srv_cert_len;
    p_sec_cfg->tls_srv_key_len = tls_srv_key_len;
    p_sec_cfg->tls_ver_max = MBEDTLS_SSL_VERSION_TLS1_3;
    p_sec_cfg->tls_ver_min = MBEDTLS_SSL_VERSION_TLS1_3;
    p_sec_cfg->p_priv_pass = NULL;
    p_sec_cfg->priv_pass_len = 0;
}

/* tags array: must match the string you use in <!--#tag--> */
const char *ssi_tags[] =
{
    "temp",   /* index 0 */
    "humid"       /* index 1 (example) */
};

/* Preset values: if >= 0, the SSI handler will return these values
   instead of generating random values. Initialize to -1 (unset). */
static int g_preset_temp = -1;
static int g_preset_humid = -1;

/* API to set presets (can be called from elsewhere in the application) */
void http_set_presets(int temp, int humid)
{
    g_preset_temp = temp;
    g_preset_humid = humid;
}

/* tSSIHandler prototype (lwIP): returns number of bytes written into pcInsert */
u16_t ssi_handler(int idx, char *insert, int insert_len)
{
    int n ;
    int randomNumberInRange;
    int val;

    APP_PRINT_INFO("[%s:%d] tag_idx = %d", __func__, __LINE__, idx);
    switch(idx)
    {
        case 0:        // temp
        {
            /* If a preset temperature is set, use it; otherwise generate a random value. */
            if (g_preset_temp >= 0)
            {
                val = g_preset_temp;
                n = snprintf(insert, insert_len, "%d C", val);
                APP_PRINT_INFO("[SSI][temp] using preset %d", val);
            }
            else
            {
                randomNumberInRange = (rand() % 41) + 10; /* 10..50 C */
                n = snprintf(insert, insert_len, "%d C", randomNumberInRange);
                APP_PRINT_INFO("[INFO][SSI][temp] generated random %d", randomNumberInRange);
            }

            if (n < 0)
            {
            	return 0;
            }

            if (n >= insert_len)
            {
            	n = insert_len - 1;
            }

            /* log the final string we inserted */
            APP_PRINT_INFO("[SSI][temp] insert=\"%.*s\"", n, insert);
            return (u16_t)n;
        }

        case 1: // humidity
        {
            /* If a preset humidity is set, use it; otherwise generate a random value. */
            if (g_preset_humid >= 0)
            {
                val = g_preset_humid;
                n = snprintf(insert, insert_len, "%d%%", val);
                APP_PRINT_INFO("[SSI][humid] using preset %d", val);
            }
            else
            {
                randomNumberInRange = (rand() % 81) + 10; /* 10..90 % */
                n = snprintf(insert, insert_len, "%d%%", randomNumberInRange);
                APP_PRINT_INFO("[SSI][humid] generated random %d", randomNumberInRange);
            }

            if (n < 0)
            {
            	return 0;
            }

            if (n >= insert_len)
            {
            	n = insert_len - 1;
            }

            /* log the final string we inserted */
            APP_PRINT_INFO("[SSI][humid] insert=\"%.*s\"", n, insert);
            return (u16_t)n;
        }

        default:
            insert[0] = '\0';
    }

    return 0;
}

/* Provide a small API to produce a JSON payload with the current values.
 * Returns number of bytes written (excluding terminating NUL). */
static int http_get_values_json(char *buf, int buflen, const char *key)
{
    int ntemp, nhumid;
    char temp_str[32];
    char humid_str[32];
    int n;
    int randomNumberInRange;

    if (!buf || buflen <= 0)
    {
        return 0;
    }

    /* If presets are set, use them; otherwise generate sample values like ssi_handler */
    if (g_preset_temp >= 0)
    {
        ntemp = g_preset_temp;
        snprintf(temp_str, sizeof(temp_str), "%d C", ntemp);
    }
    else
    {
        randomNumberInRange = (rand() % 41) + 10; /* 10..50 C */
        ntemp = randomNumberInRange;
        snprintf(temp_str, sizeof(temp_str), "%d C", ntemp);
    }

    if (g_preset_humid >= 0)
    {
        nhumid = g_preset_humid;
        snprintf(humid_str, sizeof(humid_str), "%d%%", nhumid);
    }
    else
    {
        randomNumberInRange = (rand() % 81) + 10; /* 10..90 % */
        nhumid = randomNumberInRange;
        snprintf(humid_str, sizeof(humid_str), "%d%%", nhumid);
    }

    /* If caller requested a single key, produce JSON containing only that key */
    if (key && key[0])
    {
        /* temp (string) */
        if (strcmp(key, "temp") == 0) {
            int ret = snprintf(buf, (size_t)buflen, "{\"temp\":\"%s\"}", temp_str);
            if (ret < 0)
            {
                return 0;
            }
            if (ret >= buflen)
            {
                ret = buflen - 1;
            }
            buf[ret] = '\0';
            return ret;
        }
        /* temp_value (numeric) */
        if (strcmp(key, "temp_value") == 0) {
            int ret = snprintf(buf, (size_t)buflen, "{\"temp_value\":%d}", ntemp);
            if (ret < 0)
            {
                return 0;
            }
            if (ret >= buflen)
            {
                ret = buflen - 1;
            }
            buf[ret] = '\0';
            return ret;
        }
        /* humid (string) */
        if (strcmp(key, "humid") == 0) {
            int ret = snprintf(buf, (size_t)buflen, "{\"humid\":\"%s\"}", humid_str);
            if (ret < 0)
            {
                return 0;
            }
            if (ret >= buflen)
            {
                ret = buflen - 1;
            }
            buf[ret] = '\0';
            return ret;
        }
        /* humid_value (numeric) */
        if (strcmp(key, "humid_value") == 0) {
            int ret = snprintf(buf, (size_t)buflen, "{\"humid_value\":%d}", nhumid);
            if (ret < 0)
            {
                return 0;
            }
            if (ret >= buflen)
            {
                ret = buflen - 1;
            }
            buf[ret] = '\0';
            return ret;
        }
        /* files (array) */
        if (strcmp(key, "files") == 0) {
            int ret = 0;
            int rem = buflen;
            char *p = buf;
            int wrote = snprintf(p, (size_t)rem, "{\"files\":[" );
            if (wrote < 0)
            {
                wrote = 0;
            }
            if (wrote >= rem)
            {
                wrote = rem - 1;
            }
            p += wrote;
            rem -= wrote;
            ret += wrote;

            if (lfs_mutex_get() && (lfs_mutex_take(pdMS_TO_TICKS(500)) == pdTRUE)) {
                lfs_dir_t d; struct lfs_info info; int first = 1;
                if (lfs_dir_open(&g_rm_littlefs0_lfs, &d, "/") == 0) {
                    const int MAX_FILES = 1024; int filecount = 0;
                    while (filecount < MAX_FILES) {
                        int r = lfs_dir_read(&g_rm_littlefs0_lfs, &d, &info);
                        if (r <= 0)
                        {
                            break;
                        }
                        if ((strcmp(info.name, ".") == 0) || (strcmp(info.name, "..") == 0))
                        {
                            continue;
                        }
                        if (!first)
                        {
                            if (rem > 1)
                            {
                                *p++ = ',';
                                rem--;
                                ret++;
                            }
                            else
                            {
                                break;
                            }
                        }
                        int need = snprintf(p, (size_t)rem, "{\"name\":\"%s\",\"type\":\"%s\",\"size\":%u}",
                                           info.name, (info.type == LFS_TYPE_DIR) ? "dir" : "file", (unsigned)info.size);
                        if (need < 0)
                        {
                            need = 0;
                        }
                        if (need >= rem)
                        {
                            break;
                        }
                        p += need;
                        rem -= need;
                        ret += need;
                        first = 0;
                        filecount++;
                    }
                    lfs_dir_close(&g_rm_littlefs0_lfs, &d);
                }
                lfs_mutex_give();
            }

            if (rem > 0)
            {
                int w2 = snprintf(p, (size_t)rem, "]}");
                if (w2 > 0)
                {
                    p += w2;
                    rem -= w2;
                    ret += w2;
                }
            }
            if (buflen > 0)
            {
                buf[(ret < buflen) ? ret : (buflen-1)] = '\0';
            }
            return ret;
        }
        /* fs_total or fs_free */
        if ((strcmp(key, "fs_total") == 0) || (strcmp(key, "fs_free") == 0)) {
            unsigned long total_bytes = (unsigned long) g_rm_littlefs0_lfs_cfg.block_size * (unsigned long) g_rm_littlefs0_lfs_cfg.block_count;
            unsigned long free_bytes = 0UL;
            if (lfs_mutex_get() && (lfs_mutex_take(pdMS_TO_TICKS(500)) == pdTRUE)) {
                lfs_ssize_t used_blocks = lfs_fs_size(&g_rm_littlefs0_lfs);
                if (used_blocks >= 0)
                {
                    unsigned long used_bytes = (unsigned long) used_blocks * (unsigned long) g_rm_littlefs0_lfs_cfg.block_size;
                    if (used_bytes >= total_bytes)
                    {
                        free_bytes = 0;
                    }
                    else
                    {
                        free_bytes = total_bytes - used_bytes;
                    }
                }
                lfs_mutex_give();
            }
            if (strcmp(key, "fs_total") == 0) {
                int ret = snprintf(buf, (size_t)buflen, "{\"fs_total\":%lu}", total_bytes);
                if (ret < 0)
                {
                    return 0;
                }
                if (ret >= buflen)
                {
                    ret = buflen - 1;
                }
                buf[ret] = '\0';
                return ret;
            } else {
                int ret = snprintf(buf, (size_t)buflen, "{\"fs_free\":%lu}", free_bytes);
                if (ret < 0)
                {
                    return 0;
                }
                if (ret >= buflen)
                {
                    ret = buflen - 1;
                }
                buf[ret] = '\0';
                return ret;
            }
        }
        /* unknown key -> empty object */
        int ret0 = snprintf(buf, (size_t)buflen, "{}");
        if (ret0 < 0)
        {
            return 0;
        }
        if (ret0 >= buflen)
        {
            ret0 = buflen-1;
        }
        buf[ret0]='\0';
        return ret0;
    }

    /* Build JSON: simple object with string values matching SSI formatting */
    /* Start JSON object */
    int ret = 0;
    int rem = buflen;
    char *p = buf;

    n = snprintf(p, (size_t)rem, "{\"temp\":\"%s\",\"humid\":\"%s\"",
                 temp_str, humid_str);
    if (n < 0)
    {
        return 0;
    }
    if (n >= rem)
    {
        n = rem - 1;
    }
    p += n;
    rem -= n;
    ret += n;

    /* Append file list (root directory) using LittleFS APIs. Guard with LFS mutex. */
    if (rem > 0)
    {
        int wrote = snprintf(p, (size_t)rem, ",\"files\":[" );
        if (wrote < 0)
        {
            wrote = 0;
        }
        if (wrote >= rem)
        {
            wrote = rem - 1;
        }
        p += wrote;
        rem -= wrote;
        ret += wrote;

        /* Try to acquire LFS mutex; if unavailable, skip file list */
        if (lfs_mutex_get() && (lfs_mutex_take(pdMS_TO_TICKS(500)) == pdTRUE))
        {
            lfs_dir_t d;
            struct lfs_info info;
            int first = 1;
            if (lfs_dir_open(&g_rm_littlefs0_lfs, &d, "/") == 0)
            {
                const int MAX_FILES = 64;
                int filecount = 0;
                while (filecount < MAX_FILES)
                {
                    int r = lfs_dir_read(&g_rm_littlefs0_lfs, &d, &info);
                    if (r < 0)
                    {
                        break;
                    }
                    if (r == 0)
                    {
                        break;
                    }
                    if ((strcmp(info.name, ".") == 0) || (strcmp(info.name, "..") == 0))
                    {
                        continue;
                    }

                    /* Build JSON entry for this file */
                    if (!first)
                    {
                        if (rem > 1)
                        {
                            *p++ = ',';
                            rem--;
                            ret++;
                        }
                        else
                        {
                            break;
                        }
                    }

                    /* Ensure we don't exceed buffer: conservative write */
                    int need = snprintf(p, (size_t)rem, "{\"name\":\"%s\",\"type\":\"%s\",\"size\":%u}",
                                       info.name, (info.type == LFS_TYPE_DIR) ? "dir" : "file", (unsigned)info.size);
                    if (need < 0)
                    {
                        need = 0;
                    }
                    if (need >= rem)
                    {
                        /* no room for this entry */
                        break;
                    }
                    p += need;
                    rem -= need;
                    ret += need;
                    first = 0;
                    filecount++;
                }
                lfs_dir_close(&g_rm_littlefs0_lfs, &d);
            }

            lfs_mutex_give();
        }

        /* Close files array */
        if (rem > 0)
        {
            int w2 = snprintf(p, (size_t)rem, "]");
            if (w2 > 0)
            {
                p += w2;
                rem -= w2;
                ret += w2;
            }
        }
    }

    /* Append filesystem total/free bytes using lfs_fs_size() */
    if (rem > 0)
    {
        unsigned long total_bytes = (unsigned long) g_rm_littlefs0_lfs_cfg.block_size * (unsigned long) g_rm_littlefs0_lfs_cfg.block_count;
        unsigned long free_bytes = 0UL;

        if (lfs_mutex_get() && (lfs_mutex_take(pdMS_TO_TICKS(500)) == pdTRUE))
        {
            lfs_ssize_t used_blocks = lfs_fs_size(&g_rm_littlefs0_lfs);
            if (used_blocks >= 0)
            {
                unsigned long used_bytes = (unsigned long) used_blocks * (unsigned long) g_rm_littlefs0_lfs_cfg.block_size;
                if (used_bytes >= total_bytes)
                {
                    free_bytes = 0;
                }
                else
                {
                    free_bytes = total_bytes - used_bytes;
                }
            }
            lfs_mutex_give();
        }

        int w3 = snprintf(p, (size_t)rem, ",\"fs_total\":%lu,\"fs_free\":%lu", total_bytes, free_bytes);
        if (w3 < 0)
        {
            w3 = 0;
        }
        if (w3 >= rem)
        {
            w3 = rem - 1;
        }
        p += w3;
        rem -= w3;
        ret += w3;
    }

    /* Close JSON object */
    if (rem > 0)
    {
        int w4 = snprintf(p, (size_t)rem, "}");
        if (w4 > 0)
        {
            p += w4;
            rem -= w4;
            ret += w4;
        }
    }

    /* Ensure buffer NUL termination if space allows */
    if (buflen > 0)
    {
        buf[(ret < buflen) ? ret : (buflen-1)] = '\0';
    }
    return ret;
}

/* Public wrapper that forwards to the concrete generator. Kept for
 * backward compatibility and for modules that want to ask for a
 * specific key directly. */
int http_get_values_for_key(char *buf, int buflen, const char *key)
{
    return http_get_values_json(buf, buflen, key);
}

/* Parse a /get_values URI and extract an optional key.
 * Supports:
 *   /get_values
 *   /get_values?key=foo
 *   /get_values/foo
 * Also accepts being passed an entire request path such as "/get_values/temp_value".
 */
int http_get_values_for_uri(char *buf, int buflen, const char *uri)
{
    if (!buf || buflen <= 0)
    {
        return 0;
    }

    const char *keyptr = NULL;
    char keybuf[64];

    if (uri && uri[0]) {
        /* Path-style: /get_values/<key> */
        if (strncmp(uri, "/get_values/", 12) == 0) {
            const char *k = uri + 12;
            size_t i = 0;
            while (k[i] && k[i] != '/' && k[i] != '?' && k[i] != '&' && i < (sizeof(keybuf)-1))
            {
                keybuf[i] = k[i];
                i++;
            }
            keybuf[i] = '\0';
            if (i > 0)
            {
                keyptr = keybuf;
            }
        } else {
            /* Query-style: /get_values?key=foo or /get_values?foo */
            const char *q = strchr(uri, '?');
            if (q && q[1])
            {
                const char *k = q + 1;
                if (strncmp(k, "key=", 4) == 0)
                {
                    k += 4;
                }
                size_t i = 0;
                while (k[i] && k[i] != '&' && i < (sizeof(keybuf)-1))
                {
                    keybuf[i] = k[i];
                    i++;
                }
                keybuf[i] = '\0';
                if (i > 0)
                {
                    keyptr = keybuf;
                }
            }
        }

        /* If still not found, scan for known tokens anywhere in the URI (robustness for servers that mangle queries) */
        if (!keyptr) {
            const char *known_keys[] = { "temp_value", "temp", "humid_value", "humid", "files", "fs_total", "fs_free", NULL };
            for (const char **kp = known_keys; *kp; kp++) {
                const char *found = strstr(uri, *kp);
                if (found) {
                    strncpy(keybuf, *kp, sizeof(keybuf)-1);
                    keybuf[sizeof(keybuf)-1] = '\0';
                    keyptr = keybuf;
                    break;
                }
            }
        }
    }

    return http_get_values_json(buf, buflen, keyptr);
}

/* CGI handler to set presets via: /set_presets?temp=NN&humid=MM
 * Returns the path of the page to serve after setting presets. */
static const char *cgi_set_presets(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    int temp = -1;
    int humid = -1;

    for (int i = 0; i < iNumParams; i++)
    {
        if (pcParam[i] == NULL || pcValue[i] == NULL)
        {
            continue;
        }

        if (strcmp(pcParam[i], "temp") == 0)
        {
            temp = atoi(pcValue[i]);
        }
        else if (strcmp(pcParam[i], "humid") == 0)
        {
            humid = atoi(pcValue[i]);
        }
    }

    /* Apply presets (use -1 to unset) */
    http_set_presets(temp, humid);
    APP_PRINT_INFO("[INFO][CGI] set_presets temp=%d humid=%d", temp, humid);

    /* Return a page to show after applying presets (dynamic page will show new values) */
    return "/dynamic_render.shtml";
}

fsp_err_t init_server()
{
    fsp_err_t err = FSP_SUCCESS;

    /* Open the HTTPS module. */
    err = RM_HTTPS_W_Open((https_ctrl_t *) &g_https_w0_ctrl, &g_https_w0_cfg);
    if (err != FSP_SUCCESS)
    {
        APP_PRINT_ERR("Error: Unable to open http module\n");

        return err;
    }

     config_secure_connection(&p_sec);
#if LWIP_HTTPD_SSI
     /* Register SSI handler before starting the server so SSI tags are handled
         immediately when the server starts receiving requests. */
     http_set_ssi_handler(ssi_handler, ssi_tags, sizeof(ssi_tags)/sizeof(ssi_tags[0]));
#endif
#if LWIP_HTTPD_CGI
     /* Register CGI handlers - allow setting presets via GET: /set_presets?temp=NN&humid=MM */
     {
         static const tCGI cgis[] =
         {
             { "/set_presets", cgi_set_presets },
         };
         http_set_cgi_handlers(cgis, (int) (sizeof(cgis) / sizeof(cgis[0])));
     }
#endif
     err = RM_HTTPS_W_ServerStart((https_ctrl_t *) &g_https_w0_ctrl, &p_sec);

    if (err != FSP_SUCCESS)
    {
        APP_PRINT_ERR("Error: Http Server start failed\n");
    }
    else
    {
        APP_PRINT_INFO("Https Server Running...\n");
    }

    return err;
}

void deinit_server()
{
    APP_PRINT_INFO("Https Server closing...\n");
    RM_HTTPS_W_ServerStop((https_ctrl_t *) &g_https_w0_ctrl);
    RM_HTTPS_W_Close((https_ctrl_t *) &g_https_w0_ctrl);
}

void format_fs()
{
    int rc;

    lfs_format(&g_rm_littlefs0_lfs, &g_rm_littlefs0_lfs_cfg);
    rc = lfs_mount(&g_rm_littlefs0_lfs, &g_rm_littlefs0_lfs_cfg);
    if (rc < 0)
    {
        APP_PRINT_ERR("\n\rFailed to lfs_mount file system (%d)\n", rc);
    }
}

int write_file(char *p_path, char *p_buf)
{
    int lfs_rc;
    int wr;
    int err_val = 0;
    lfs_file_t file;

    /* Open file for writing (truncate if exists) */
    lfs_rc = lfs_file_open(&g_rm_littlefs0_lfs, &file, p_path,
                          (LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC));
    if (lfs_rc < 0)
    {
        return lfs_rc;
    }

    wr = lfs_file_write(&g_rm_littlefs0_lfs, &file, p_buf, strlen(p_buf));

    if (wr < 0)
    {
        err_val = wr;
    }

    lfs_file_close(&g_rm_littlefs0_lfs, &file);

    return err_val;
}
