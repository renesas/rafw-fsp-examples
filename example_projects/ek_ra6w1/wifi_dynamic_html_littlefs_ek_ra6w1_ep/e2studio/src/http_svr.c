/***********************************************************************************************************************
* File Name    : http_srv.c
* Description  : http(s) server functions and configurations
**********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "common_data.h"
#include "common_utils.h"
#include "http_svr.h"
#include "lfs.h"
#include "rm_httpd.h"
#include "i2c_sensor.h"

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

const char *ssi_tags[] =
{
    "temp",   /* index 0 */
    "humid"   /* index 1 */
};


static volatile float g_cached_temp = -999.0f;

/* Preset values: if >= 0, SSI/JSON handlers use these directly.
 * Both are set by http_update_sensor_data() after each sensor read. */
static volatile int g_preset_temp  = -1;
static volatile int g_preset_humid = -1;

/* Called from app_task thread ONLY — reads I2C sensor and caches result */
void http_update_sensor_data(void)
{
    uint8_t  raw[SENSOR_DATA_SIZE] = {RESET_VALUE};

    /* --- Temperature calibration & raw read --- */
    uint16_t t1 = read_reg_data(REGISTER_DIG_T1);
    int16_t  t2 = (int16_t)read_reg_data(REGISTER_DIG_T2);
    int16_t  t3 = (int16_t)read_reg_data(REGISTER_DIG_T3);

    APP_PRINT_INFO("[sensor] T1=0x%x T2=0x%x T3=0x%x\n", t1, t2, t3);

    fsp_err_t err = read_sensor_data(raw);
    if (FSP_SUCCESS != err)
    {
        APP_PRINT_ERR("[sensor] read_sensor_data failed\n");
        return;
    }

    int32_t adc_T = ((int32_t)raw[2])
                  | ((int32_t)raw[1] << 8)
                  | ((int32_t)raw[0] << 16);
    adc_T >>= 4;

    int32_t var1 = ((adc_T / 8) - ((int32_t)t1 * 2));
    var1 = (var1 * (int32_t)t2) / 2048;

    int32_t var2 = ((adc_T / 16) - (int32_t)t1);
    var2 = (((var2 * var2) / 4096) * (int32_t)t3) / 16384;

    /* t_fine is shared by the humidity compensation formula */
    int32_t t_fine = var1 + var2;

    int32_t T = (t_fine * 5 + 128) / 256;
    float real_temp = (float)T / 100.0f;

    int32_t t_int  = (int32_t)real_temp;
    int32_t t_frac = (int32_t)((real_temp - (float)t_int) * 100.0f);
    APP_PRINT_INFO("[sensor] Temperature: %d.%02d C\n", (int)t_int, (int)t_frac);

    g_cached_temp = real_temp;
    g_preset_temp = (int)real_temp;

    /* --- Humidity calibration --- */
    /* dig_H1: single byte at 0xA1 — use read_reg_data, take low byte only */
    uint8_t  dig_H1 = (uint8_t)(read_reg_data(REGISTER_DIG_H1) & 0xFF);

    /* dig_H2..H6: packed in 0xE1–0xE7, read via read_reg_data pairs */
    uint16_t e1_e2  = read_reg_data(REGISTER_DIG_H2);   /* 0xE1=LSB, 0xE2=MSB */
    int16_t  dig_H2 = (int16_t)e1_e2;

    uint16_t e3_e4  = read_reg_data(REGISTER_DIG_H3);   /* 0xE3=H3, 0xE4=H4[11:4] */
    uint8_t  dig_H3 = (uint8_t)(e3_e4 & 0xFF);
    uint8_t  e4     = (uint8_t)((e3_e4 >> 8) & 0xFF);

    uint16_t e5_e6  = read_reg_data(REGISTER_DIG_H5);   /* 0xE5=H4[3:0]|H5[3:0], 0xE6=H5[11:4] */
    uint8_t  e5     = (uint8_t)(e5_e6 & 0xFF);
    uint8_t  e6     = (uint8_t)((e5_e6 >> 8) & 0xFF);

    int16_t  dig_H4 = (int16_t)(((int16_t)e4 << 4) | (e5 & 0x0F));
    int16_t  dig_H5 = (int16_t)(((int16_t)e6 << 4) | ((e5 >> 4) & 0x0F));

    uint16_t e7_xx  = read_reg_data(REGISTER_DIG_H6);
    int8_t   dig_H6 = (int8_t)(e7_xx & 0xFF);

    APP_PRINT_INFO("[sensor] H1=%u H2=%d H3=%u H4=%d H5=%d H6=%d\n",
                   dig_H1, dig_H2, dig_H3, dig_H4, dig_H5, dig_H6);

    /* --- Humidity raw read --- */
    uint16_t raw_h = 0;
    err = read_humidity_raw(&raw_h);
    if (FSP_SUCCESS != err || raw_h == 0x8000U)
    {
        APP_PRINT_ERR("[sensor] Humidity read failed or skipped (raw=0x%04X)\n", raw_h);
        return;
    }

    /* --- BME280 humidity compensation --- */
    /* v   = t_fine - 76800  (used in BOTH factors of the big multiply)
     * x   = first factor: (adc_H scaled - H4/H5 offset) >> 15
     * The grouping (inner * H2 + 8192) >> 14 keeps intermediates in int32 range.
     * Splitting x * H2 into a separate step causes ~46 billion overflow → 0. */
    int32_t adc_H = (int32_t)raw_h;
    int32_t v = t_fine - 76800;           /* saved; used in both factor and multiplier */
    int32_t x = (((adc_H << 14) - ((int32_t)dig_H4 << 20) - ((int32_t)dig_H5 * v)) + 16384) >> 15;

    int32_t inner = (((((v * (int32_t)dig_H6) >> 10) *
                       (((v * (int32_t)dig_H3) >> 11) + 32768)) >> 10) + 2097152);
    x = x * ((inner * (int32_t)dig_H2 + 8192) >> 14);

    x -= ((((x >> 15) * (x >> 15)) >> 7) * (int32_t)dig_H1) >> 4;
    if (x < 0)         x = 0;
    if (x > 419430400) x = 419430400;
    /* Result in %RH × 1024 */
    int32_t humidity_q10 = x >> 12;                /* %RH × 1024 */
    int32_t hum_int  = humidity_q10 / 1024;
    int32_t hum_frac = ((humidity_q10 % 1024) * 100) / 1024;

    APP_PRINT_INFO("[sensor] Humidity: %d.%02d %%RH\n", (int)hum_int, (int)hum_frac);
    g_preset_humid = (int)hum_int;
}

/* API to set presets manually via CGI */
void http_set_presets(int temp, int humid)
{
    if (temp >= 0)  g_preset_temp  = temp;
    if (humid >= 0) g_preset_humid = humid;
}

u16_t ssi_handler(int idx, char *insert, int insert_len)
{
    int n;
    int val;

    APP_PRINT_INFO("[%s:%d] tag_idx = %d", __func__, __LINE__, idx);
    switch(idx)
    {
        case 0:  /* temp */
        {
            val = g_preset_temp;
            if (val >= 0)
            {
                n = snprintf(insert, insert_len, "%d C", val);
                APP_PRINT_INFO("[SSI][temp] %d C", val);
            }
            else
            {
                n = snprintf(insert, insert_len, "-- C");
            }

            if (n < 0)        return 0;
            if (n >= insert_len) n = insert_len - 1;
            return (u16_t)n;
        }

        case 1:  /* humid */
        {
            val = g_preset_humid;
            if (val >= 0)
            {
                n = snprintf(insert, insert_len, "%d%%", val);
                APP_PRINT_INFO("[SSI][humid] %d%%", val);
            }
            else
            {
                n = snprintf(insert, insert_len, "-- %%");
            }

            if (n < 0)        return 0;
            if (n >= insert_len) n = insert_len - 1;
            return (u16_t)n;
        }

        default:
            insert[0] = '\0';
    }

    return 0;
}


static int http_get_values_json(char *buf, int buflen, const char *key)
{
    int ntemp, nhumid;
    char temp_str[32];
    char humid_str[32];
    int n;

    if (!buf || buflen <= 0)
        return 0;

    /* Read from cache only — never touch I2C here */
    ntemp = g_preset_temp;
    if (ntemp >= 0)
        snprintf(temp_str, sizeof(temp_str), "%d C", ntemp);
    else
        snprintf(temp_str, sizeof(temp_str), "-- C");

    nhumid = g_preset_humid;
    if (nhumid >= 0)
        snprintf(humid_str, sizeof(humid_str), "%d%%", nhumid);
    else
        snprintf(humid_str, sizeof(humid_str), "--%%");

    /* Single-key responses */
    if (key && key[0])
    {
        if (strcmp(key, "temp") == 0) {
            int ret = snprintf(buf, (size_t)buflen, "{\"temp\":\"%s\"}", temp_str);
            if (ret < 0) return 0;
            if (ret >= buflen) ret = buflen - 1;
            buf[ret] = '\0';
            return ret;
        }
        if (strcmp(key, "temp_value") == 0) {
            int ret = snprintf(buf, (size_t)buflen, "{\"temp_value\":%d}", ntemp);
            if (ret < 0) return 0;
            if (ret >= buflen) ret = buflen - 1;
            buf[ret] = '\0';
            return ret;
        }
        if (strcmp(key, "humid") == 0) {
            int ret = snprintf(buf, (size_t)buflen, "{\"humid\":\"%s\"}", humid_str);
            if (ret < 0) return 0;
            if (ret >= buflen) ret = buflen - 1;
            buf[ret] = '\0';
            return ret;
        }
        if (strcmp(key, "humid_value") == 0) {
            int ret = snprintf(buf, (size_t)buflen, "{\"humid_value\":%d}", nhumid);
            if (ret < 0) return 0;
            if (ret >= buflen) ret = buflen - 1;
            buf[ret] = '\0';
            return ret;
        }
        if (strcmp(key, "files") == 0) {
            int ret = 0;
            int rem = buflen;
            char *p = buf;
            int wrote = snprintf(p, (size_t)rem, "{\"files\":[");
            if (wrote < 0) wrote = 0;
            if (wrote >= rem) wrote = rem - 1;
            p += wrote; rem -= wrote; ret += wrote;

            if (lfs_mutex_get() && (lfs_mutex_take(pdMS_TO_TICKS(500)) == pdTRUE)) {
                lfs_dir_t d; struct lfs_info info; int first = 1;
                if (lfs_dir_open(&g_rm_littlefs0_lfs, &d, "/") == 0) {
                    const int MAX_FILES = 64; int filecount = 0;
                    while (filecount < MAX_FILES) {
                        int r = lfs_dir_read(&g_rm_littlefs0_lfs, &d, &info);
                        if (r <= 0) break;
                        if ((strcmp(info.name, ".") == 0) || (strcmp(info.name, "..") == 0)) continue;
                        if (!first) {
                            if (rem > 1) { *p++ = ','; rem--; ret++; } else break;
                        }
                        int need = snprintf(p, (size_t)rem, "{\"name\":\"%s\",\"type\":\"%s\",\"size\":%u}",
                                           info.name, (info.type == LFS_TYPE_DIR) ? "dir" : "file", (unsigned)info.size);
                        if (need < 0) need = 0;
                        if (need >= rem) break;
                        p += need; rem -= need; ret += need;
                        first = 0; filecount++;
                    }
                    lfs_dir_close(&g_rm_littlefs0_lfs, &d);
                }
                lfs_mutex_give();
            }
            if (rem > 0) {
                int w2 = snprintf(p, (size_t)rem, "]}");
                if (w2 > 0) { p += w2; rem -= w2; ret += w2; }
            }
            if (buflen > 0) buf[(ret < buflen) ? ret : (buflen-1)] = '\0';
            return ret;
        }
        if ((strcmp(key, "fs_total") == 0) || (strcmp(key, "fs_free") == 0)) {
            unsigned long total_bytes = (unsigned long)g_rm_littlefs0_lfs_cfg.block_size *
                                        (unsigned long)g_rm_littlefs0_lfs_cfg.block_count;
            unsigned long free_bytes = 0UL;
            if (lfs_mutex_get() && (lfs_mutex_take(pdMS_TO_TICKS(500)) == pdTRUE)) {
                lfs_ssize_t used_blocks = lfs_fs_size(&g_rm_littlefs0_lfs);
                if (used_blocks >= 0) {
                    unsigned long used_bytes = (unsigned long)used_blocks *
                                               (unsigned long)g_rm_littlefs0_lfs_cfg.block_size;
                    free_bytes = (used_bytes >= total_bytes) ? 0 : total_bytes - used_bytes;
                }
                lfs_mutex_give();
            }
            int ret;
            if (strcmp(key, "fs_total") == 0)
                ret = snprintf(buf, (size_t)buflen, "{\"fs_total\":%lu}", total_bytes);
            else
                ret = snprintf(buf, (size_t)buflen, "{\"fs_free\":%lu}", free_bytes);
            if (ret < 0) return 0;
            if (ret >= buflen) ret = buflen - 1;
            buf[ret] = '\0';
            return ret;
        }
        /* unknown key */
        int ret0 = snprintf(buf, (size_t)buflen, "{}");
        if (ret0 < 0) return 0;
        if (ret0 >= buflen) ret0 = buflen - 1;
        buf[ret0] = '\0';
        return ret0;
    }

    /* Full JSON object */
    int ret = 0;
    int rem = buflen;
    char *p = buf;

    n = snprintf(p, (size_t)rem, "{\"temp\":\"%s\",\"humid\":\"%s\"", temp_str, humid_str);
    if (n < 0) return 0;
    if (n >= rem) n = rem - 1;
    p += n; rem -= n; ret += n;

    if (rem > 0) {
        int wrote = snprintf(p, (size_t)rem, ",\"files\":[");
        if (wrote < 0) wrote = 0;
        if (wrote >= rem) wrote = rem - 1;
        p += wrote; rem -= wrote; ret += wrote;

        if (lfs_mutex_get() && (lfs_mutex_take(pdMS_TO_TICKS(500)) == pdTRUE)) {
            lfs_dir_t d; struct lfs_info info; int first = 1;
            if (lfs_dir_open(&g_rm_littlefs0_lfs, &d, "/") == 0) {
                const int MAX_FILES = 64; int filecount = 0;
                while (filecount < MAX_FILES) {
                    int r = lfs_dir_read(&g_rm_littlefs0_lfs, &d, &info);
                    if (r < 0 || r == 0) break;
                    if ((strcmp(info.name, ".") == 0) || (strcmp(info.name, "..") == 0)) continue;
                    if (!first) {
                        if (rem > 1) { *p++ = ','; rem--; ret++; } else break;
                    }
                    int need = snprintf(p, (size_t)rem, "{\"name\":\"%s\",\"type\":\"%s\",\"size\":%u}",
                                       info.name, (info.type == LFS_TYPE_DIR) ? "dir" : "file", (unsigned)info.size);
                    if (need < 0) need = 0;
                    if (need >= rem) break;
                    p += need; rem -= need; ret += need;
                    first = 0; filecount++;
                }
                lfs_dir_close(&g_rm_littlefs0_lfs, &d);
            }
            lfs_mutex_give();
        }

        if (rem > 0) {
            int w2 = snprintf(p, (size_t)rem, "]");
            if (w2 > 0) { p += w2; rem -= w2; ret += w2; }
        }
    }

    if (rem > 0) {
        unsigned long total_bytes = (unsigned long)g_rm_littlefs0_lfs_cfg.block_size *
                                    (unsigned long)g_rm_littlefs0_lfs_cfg.block_count;
        unsigned long free_bytes = 0UL;
        if (lfs_mutex_get() && (lfs_mutex_take(pdMS_TO_TICKS(500)) == pdTRUE)) {
            lfs_ssize_t used_blocks = lfs_fs_size(&g_rm_littlefs0_lfs);
            if (used_blocks >= 0) {
                unsigned long used_bytes = (unsigned long)used_blocks *
                                           (unsigned long)g_rm_littlefs0_lfs_cfg.block_size;
                free_bytes = (used_bytes >= total_bytes) ? 0 : total_bytes - used_bytes;
            }
            lfs_mutex_give();
        }
        int w3 = snprintf(p, (size_t)rem, ",\"fs_total\":%lu,\"fs_free\":%lu", total_bytes, free_bytes);
        if (w3 < 0) w3 = 0;
        if (w3 >= rem) w3 = rem - 1;
        p += w3; rem -= w3; ret += w3;
    }

    if (rem > 0) {
        int w4 = snprintf(p, (size_t)rem, "}");
        if (w4 > 0) { p += w4; rem -= w4; ret += w4; }
    }

    if (buflen > 0) buf[(ret < buflen) ? ret : (buflen-1)] = '\0';
    return ret;
}

int http_get_values_for_key(char *buf, int buflen, const char *key)
{
    return http_get_values_json(buf, buflen, key);
}

int http_get_values_for_uri(char *buf, int buflen, const char *uri)
{
    if (!buf || buflen <= 0) return 0;

    const char *keyptr = NULL;
    char keybuf[64];

    if (uri && uri[0]) {
        if (strncmp(uri, "/get_values/", 12) == 0) {
            const char *k = uri + 12;
            size_t i = 0;
            while (k[i] && k[i] != '/' && k[i] != '?' && k[i] != '&' && i < (sizeof(keybuf)-1))
                keybuf[i++] = k[i];
            keybuf[i] = '\0';
            if (i > 0) keyptr = keybuf;
        } else {
            const char *q = strchr(uri, '?');
            if (q && q[1]) {
                const char *k = q + 1;
                if (strncmp(k, "key=", 4) == 0) k += 4;
                size_t i = 0;
                while (k[i] && k[i] != '&' && i < (sizeof(keybuf)-1))
                    keybuf[i++] = k[i];
                keybuf[i] = '\0';
                if (i > 0) keyptr = keybuf;
            }
        }
        if (!keyptr) {
            const char *known_keys[] = { "temp_value", "temp", "humid_value", "humid", "files", "fs_total", "fs_free", NULL };
            for (const char **kp = known_keys; *kp; kp++) {
                if (strstr(uri, *kp)) {
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

static const char *cgi_set_presets(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    (void)iIndex;
    int temp = -1;
    int humid = -1;

    for (int i = 0; i < iNumParams; i++) {
        if (pcParam[i] == NULL || pcValue[i] == NULL) continue;
        if (strcmp(pcParam[i], "temp") == 0)  temp  = atoi(pcValue[i]);
        else if (strcmp(pcParam[i], "humid") == 0) humid = atoi(pcValue[i]);
    }

    http_set_presets(temp, humid);
    APP_PRINT_INFO("[CGI] set_presets temp=%d humid=%d", temp, humid);
    return "/dynamic_render.shtml";
}

fsp_err_t init_server()
{
    fsp_err_t err = FSP_SUCCESS;

    err = RM_HTTPS_W_Open((https_ctrl_t *) &g_https_w0_ctrl, &g_https_w0_cfg);
    if (err != FSP_SUCCESS)
    {
        APP_PRINT_ERR("Error: Unable to open http module\n");
        return err;
    }

    config_secure_connection(&p_sec);

#if LWIP_HTTPD_SSI
    http_set_ssi_handler(ssi_handler, ssi_tags, sizeof(ssi_tags)/sizeof(ssi_tags[0]));
#endif
#if LWIP_HTTPD_CGI
    {
        static const tCGI cgis[] = { { "/set_presets", cgi_set_presets } };
        http_set_cgi_handlers(cgis, (int)(sizeof(cgis) / sizeof(cgis[0])));
    }
#endif

    err = RM_HTTPS_W_ServerStart((https_ctrl_t *) &g_https_w0_ctrl, &p_sec);
    if (err != FSP_SUCCESS)
        APP_PRINT_ERR("Error: Http Server start failed\n");
    else
        APP_PRINT_INFO("Https Server Running...\n");

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
    rc = lfs_mount(&g_rm_littlefs0_lfs, &g_rm_littlefs0_lfs_cfg);
    if (rc < 0)
    {
        APP_PRINT_INFO("[fs] Mount failed (%d), formatting...\n", rc);
        lfs_format(&g_rm_littlefs0_lfs, &g_rm_littlefs0_lfs_cfg);
        rc = lfs_mount(&g_rm_littlefs0_lfs, &g_rm_littlefs0_lfs_cfg);
        if (rc < 0)
            APP_PRINT_ERR("[fs] Mount after format failed (%d)\n", rc);
        else
            APP_PRINT_INFO("[fs] Filesystem formatted and mounted OK\n");
    }
    else
    {
        APP_PRINT_INFO("[fs] Filesystem mounted OK (existing data preserved)\n");
    }
}

int write_file(char *p_path, char *p_buf)
{
    int lfs_rc;
    int wr;
    int err_val = 0;
    lfs_file_t file;

    lfs_rc = lfs_file_open(&g_rm_littlefs0_lfs, &file, p_path,
                           (LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC));
    if (lfs_rc < 0) return lfs_rc;

    wr = lfs_file_write(&g_rm_littlefs0_lfs, &file, p_buf, strlen(p_buf));
    if (wr < 0) err_val = wr;

    lfs_file_close(&g_rm_littlefs0_lfs, &file);
    return err_val;
}
