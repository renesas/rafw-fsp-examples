/***********************************************************************************************************************
* File Name    : common_support.h
* Description  : Common support function declarations.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#ifndef _MAIN_SUPPORT_H_
#define _MAIN_SUPPORT_H_

void RemoveCRLF(char * str);
int make_argv(char *s, int argvsz, char *argv[]);
void AnalyzeCommand(int argc, char* argv[]);
#endif/*_MAIN_SUPPORT_H_*/
