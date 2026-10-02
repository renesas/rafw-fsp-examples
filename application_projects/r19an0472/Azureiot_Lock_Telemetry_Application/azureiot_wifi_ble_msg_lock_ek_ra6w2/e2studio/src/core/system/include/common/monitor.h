/**********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *********************************************************************************************************************/
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
