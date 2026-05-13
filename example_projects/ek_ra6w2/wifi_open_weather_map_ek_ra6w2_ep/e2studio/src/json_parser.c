/***********************************************************************************************************************
 * File Name    : json_parser.c
 * Description  : Contains data structures and functions used in JSON parsing.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "bsp_api.h"
#include "3rd_party/cJSON.h"
#include "fsp_common_api.h"
#include "common_data.h"
#include "common_utils.h"
#include "FreeRTOS.h"

/***********************************************************************************************************************
 * Function Name: emit_json_line
 * Description  : Prints one line of indented JSON output (internal helper).
 ***********************************************************************************************************************/
static void emit_json_line(char *buf, int len, int depth)
{
    int i;
    if (len <= 0)
    {
        return;
    }
    buf[len] = '\0';
    for (i = 0; i < depth; i++)
    {
        APP_PRINT("  ");
    }
    APP_PRINT("%s\n", buf);
}

/***********************************************************************************************************************
 * Function Name: json_print
 * Description  : Formats a compact single-line JSON string with 2-space
 *                indentation.  Operates directly on the raw server bytes so
 *                float values are printed exactly as received (e.g. "76.2602")
 *                avoiding the excessive-precision artefacts of cJSON_Print.
 ***********************************************************************************************************************/
static void json_print(const char *src, int32_t len)
{
    int     depth  = 0;
    int     in_str = 0;
    int     esc    = 0;
    char    line[256];
    int     lpos   = 0;
    int32_t i;
    char    c;

    for (i = 0; i < len; i++)
    {
        c = src[i];

        if (esc)
        {
            if (lpos < 255) { line[lpos++] = c; }
            esc = 0;
            continue;
        }

        if (in_str)
        {
            if (lpos < 255) { line[lpos++] = c; }
            if (c == '\\')     { esc    = 1; }
            else if (c == '"') { in_str = 0; }
            continue;
        }

        switch (c)
        {
            case ' ': case '\t': case '\r': case '\n':
                break;

            case '"':
                in_str = 1;
                if (lpos < 255) { line[lpos++] = c; }
                break;

            case ':':
                if (lpos < 253) { line[lpos++] = ':'; line[lpos++] = ' '; line[lpos++] = ' '; }
                break;

            case '{': case '[':
                if (lpos < 255) { line[lpos++] = c; }
                emit_json_line(line, lpos, depth);
                lpos = 0;
                depth++;
                break;

            case '}': case ']':
                emit_json_line(line, lpos, depth);
                lpos = 0;
                if (depth > 0) { depth--; }
                if (lpos < 255) { line[lpos++] = c; }
                break;

            case ',':
                emit_json_line(line, lpos, depth);
                lpos = 0;
                break;

            default:
                if (lpos < 255) { line[lpos++] = c; }
                break;
        }
    }

    emit_json_line(line, lpos, depth);
}

/***********************************************************************************************************************
 * Function Name: print_wheather_info_json
 * Description  : Parses the received JSON and prints weather information.
 * Arguments    : p_args - HTTPS callback arguments containing payload
 * Return Value : FSP error code
 ***********************************************************************************************************************/
fsp_err_t print_wheather_info_json(void *p_args)
{
    /* Redirect all cJSON heap operations to the FreeRTOS heap.
     * Standard malloc is often mapped to a very small newlib heap on this
     * target, so cJSON_Parse / cJSON_Print would silently fail there. */
    static const cJSON_Hooks freertos_hooks =
    {
        .malloc_fn = pvPortMalloc,
        .free_fn   = vPortFree,
    };
    cJSON_InitHooks((cJSON_Hooks *) &freertos_hooks);

    https_callback_args_t *p = (https_callback_args_t *) p_args;
    uint8_t *payload         = (uint8_t *) p->payload;
    int32_t len              = p->len;
    u8 *json_string;
    cJSON *json;

    /* ---- Validate input ------------------------------------------------ */
    if (len <= 0)
    {
        APP_ERR_PRINT("empty payload (len=%d)\n", (int) len);
        return FSP_ERR_INVALID_ARGUMENT;
    }

    /* ---- Copy payload into a NUL-terminated string -------------------- */
    json_string = (u8 *) pvPortMalloc((size_t) len + 1U);
    if (json_string == NULL)
    {
        APP_ERR_PRINT("pvPortMalloc failed for json_string (%d bytes)\n", len + 1);
        return FSP_ERR_OUT_OF_MEMORY;
    }
    memcpy(json_string, payload, (size_t) len);
    json_string[len] = '\0';

    /* ---- Parse (validates the JSON is well-formed) -------------------- */
    json = cJSON_Parse((const char *) json_string);
    if (json == NULL)
    {
        const char *err = cJSON_GetErrorPtr();
        if (err != NULL)
        {
            APP_ERR_PRINT("JSON parse error near: %.32s\n", err);
        }
        else
        {
            APP_ERR_PRINT("JSON parse failed (not valid JSON)\n");
        }
        vPortFree(json_string);
        return FSP_ERR_INVALID_DATA;
    }

    /* ---- Pretty-print using raw server bytes -------------------------
     * json_pretty_print() walks json_string directly, preserving the
     * server-supplied number precision (e.g. "76.2602") and adding
     * 2-space indentation — no dependency on cJSON_Print or sprintf float.
     * ------------------------------------------------------------------ */
    APP_PRINT("weather details\n\n");
    json_print((const char *) json_string, len);

    vPortFree(json_string);
    cJSON_Delete(json);

    return FSP_SUCCESS;
}
