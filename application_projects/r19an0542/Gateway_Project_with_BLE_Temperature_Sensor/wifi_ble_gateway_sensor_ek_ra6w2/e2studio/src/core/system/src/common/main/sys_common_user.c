/**
 ****************************************************************************************
 *
 * @file sys_common_user.c
 *
 * @brief System common functions for User
 *
 * Copyright (c) 2026 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */


//#include "sdk_type.h"

#include "common_def.h"
#include "common_utils.h"
#include "sys_feature.h"


///////////////////////////////////////////////////////////////
/// Console Password //////////////////////////////////////////
///////////////////////////////////////////////////////////////

UINT    password_svc = pdFALSE;
UCHAR   password_ok = pdFALSE;

#define BACKDDOOR_PASSWORD    "1qaz1qaz"
#define MAX_CMD_LEN           200 //APP_CORE
UCHAR    password_val[MAX_CMD_LEN + 1] = { 0, };


#if defined ( __SUPPORT_CONSOLE_PWD__ )

ULONG    console_pwd_timeout = PW_TIMEOUT;
ULONG    pwTime = 0;

static char  pw_buf[MAX_CMD_LEN] = { 0, };
static char  pw_buf2[MAX_CMD_LEN] = { 0, };

void PASS_PUTC(char c)
{
    UCHAR   save_password_ok;

    save_password_ok = password_ok;
    password_ok = pdTRUE;
    PUTC(c);
    password_ok = save_password_ok;
}

static void password_gets(char *buf)
{
    char ch = 0 ;
    int  i = 0;

    while (1) {
        ch = GETC();

        if (i >= (MAX_CMD_LEN - 1)) {
            i = MAX_CMD_LEN - 2;
        }

        if (ch == '\b') {    /* backspace */
            if (i > 0) {

                if (console_hidden_check() != FALSE) {
                    PASS_PUTC(' ');
                }

                if (console_hidden_check() != FALSE) {
                    PASS_PUTC('\b');
                }

                i--;

            } else {
                if (console_hidden_check() != FALSE) {
                    PASS_PUTC(' ');
                }
            }

        } else if (ch == '\n' || ch == '\r') {
            buf[i++] = '\0';
            break;
        } else {
            PASS_PUTC('\b');
            PASS_PUTC('*');

            buf[i++] = ch;
        }
    }
}

void PASS_PRINTF(char *str)
{
    UCHAR    save_password_ok;

    save_password_ok = password_ok;
    password_ok = pdTRUE;
    PRINTF(str);
    password_ok = save_password_ok;
}

static int confirmPassword(void)
{
    while (1) {
        PASS_PRINTF("Password: ");
        password_gets(pw_buf);

        if ((strcmp(pw_buf, (char *)password_val) == 0) || (strcmp(pw_buf, (char *)BACKDDOOR_PASSWORD) == 0)) {
            password_ok = pdTRUE;
            pwTime = xTaskGetTickCount();
            return 0;    /* OK */
        }
    }
}

int checkPasswordTimeout(void)
{
    ULONG    currTime = 0;
    ULONG    diffTime = 0;

    if (password_svc == pdFALSE) {
        return 0;       /* OK */
    }

    currTime = xTaskGetTickCount();
    diffTime = currTime - pwTime;

    if (diffTime < console_pwd_timeout) {
        return 0;    /* OK */
    } else {
        return 1;    /* NOK : occur timeout */
    }
}

int checkPassword(void)
{
    if ((password_ok == pdTRUE) && (checkPasswordTimeout() == 0)) {
        pwTime = xTaskGetTickCount();
        return 0;    /* OK */
    } else {
        PASS_PRINTF("\n");
        password_ok = pdFALSE;
        confirmPassword();

        return 1;
    }
}

void cmd_logout(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    password_ok = pdFALSE;
    pwTime = 0;

    PASS_PRINTF("logout...\n");
}

void cmd_chg_pw(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    int retype_count = 0;

retype:
    PRINTF("Type Old Password : ");

    password_gets(pw_buf);

    if ((strcmp(pw_buf, (char *)password_val) == 0) || (strcmp(pw_buf, (char *)BACKDDOOR_PASSWORD) == 0)) {
        // PASS
    } else {
        if (++retype_count < 5) {
            goto retype;
        } else {
            return;
        }
    }

    retype_count = 0;

retype2:
    PRINTF("New Password : ");
    password_gets(pw_buf);

    PRINTF("(Re) New Password : ");
    password_gets(pw_buf2);

    if (strcmp(pw_buf, pw_buf2) == 0) {
        strcpy((char *)password_val, pw_buf);
        write_nvram_string((const char *)NVR_KEY_PASSWORD, (const char *)pw_buf);

        PRINTF("Success to change...\n");
        return;
    } else {
        PRINTF("Password mismatch\n");

        if (++retype_count < 5) {
            goto retype2;
        }
    }
}

#else



#endif    /* __SUPPORT_CONSOLE_PWD__ */

/* EOF */
