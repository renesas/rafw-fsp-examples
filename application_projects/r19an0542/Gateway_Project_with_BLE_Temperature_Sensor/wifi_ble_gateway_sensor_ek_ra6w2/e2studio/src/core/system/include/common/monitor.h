/**
 ****************************************************************************************
 *
 * @file monitor.h
 *
 * @brief Header files for Console Monitor
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


#ifndef __monitor_h__
#define __monitor_h__

#undef	DEBUG_DOS_KEY				/* DEBUG history command */

extern int 	make_argv(char *s, int argvsz, char *argv[]);
extern int 	console_hidden_check(void);	/* FALSE is hidden mode */
extern void print_prompt(unsigned int mode);
extern void Monitor(char *prompt);
extern void monitor_init(void);
extern void exec_mon_cmd(char *ptr);

#define MAX_ARGC_NUM        32
#if defined(__SUPPORT_AWS_IOT__) || defined(__SUPPORT_AZURE_IOT__) //APP_CORE
#define MAX_CMD_LEN		200
#else
#define MAX_CMD_LEN		160
#endif
#define	MAX_HIS_NUM		3

#ifdef __SUPPORT_CONSOLE_PWD__
extern int  write_nvram_string(const char *name, const char *val);
extern char *read_nvram_string(const char *name);
extern int checkPasswordTimeout(void);
extern int checkPassword(void);
extern void cmd_logout(int argc, char *argv[]);
extern void cmd_chg_pw(int argc, char *argv[]);
extern int  read_nvram_int(const char *name, int *_val);

extern UCHAR password_ok;
extern ULONG pwTime;
extern UCHAR password_val[MAX_CMD_LEN + 1];
extern UINT password_svc;
#endif  /* __SUPPORT_CONSOLE_PWD__ */

#endif /* __monitor_h__ */

/* EOF */
