/***********************************************************************************************************************
* File Name    : common_support.c
* Description  : Common support functions.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include <stdio.h>
#include "strings.h"
#include "stdlib.h"

#include "common_support.h"

#include "sdio_cmd.h"
#include "at_cmd.h"

#define iscmd(X)      (!strcmp(argv[0],X))
#define isatcmd(X,Y)  (!strncasecmp(argv[0],X, Y))

void RemoveCRLF(char * str)
{
    int i = 0;

    while(1) {
        if(str[i] == '\0') {
            break;
        } else if(str[i] == '\r') {
            str[i] = '\0';
        } else if(str[i] == '\n') {
            str[i] = '\0';
        }
        i++;
    }
}

int make_argv(char *s, int argvsz, char *argv[])
{
    int argc = 0;

    /* split into argv */
    while (argc < argvsz - 1) {
        /* skip any white space */
        while ((*s == ' ') || (*s == '\t') || (*s == 0x0A))
            ++s;

        if (*s == '\0') /* end of s, no more args		 */
            break;

        argv[argc++] = s; /* begin of argument string	 */

        /* find end of string */
        while (*s && (*s != ' ') && (*s != '\t') && (*s != 0x0A))
            ++s;

        if (*s == '\0') /* end of s, no more args */
            break;

        *s++ = '\0';    /* terminate current arg */
    }
    argv[argc] = NULL;

    return argc;
}

void AnalyzeCommand(int argc, char* argv[])
{
#if (SUPPORT_SPI == 1)
    Transfer_ATCmd_To_SPI(argc, argv);
#elif (SUPPORT_SDIO == 1)
    Transfer_ATCmd_To_SDIO(argc, argv);
#elif (SUPPORT_UART == 1)
    Transfer_ATCmd_To_UART(argc, argv);
#else
    FSP_PARAMETER_NOT_USED(argc);
    FSP_PARAMETER_NOT_USED(argv);
#endif
}
