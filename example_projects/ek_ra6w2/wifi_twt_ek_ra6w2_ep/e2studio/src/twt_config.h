/***********************************************************************************************************************
* File Name    : twt_config.h
* Description  : TWT parameters and WiFi events
**********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#ifndef TWT_CONFIG_H_
#define TWT_CONFIG_H_

/*
* Macros
*/
#define TWT_RTM_NAME "TWT_CONF"

#define TWT_WAKE_INT_MANTISSA 1850
#define TWT_WAKE_INT_EXPONENT 13
#define TWT_WAKE_INT_MIN_TWT_WAKE_DUR 32
#define TWT_FLOW_TYPE 1 /*0 - Announced, 1- Unannounced */
#define TWT_NEG_TYPE 0
#define TWT_TRIGGER 0
#define TWT_AUTO_SETUP 0
#define eTWTCFG 10

typedef struct
{
	int twt_session_active;   // 0 = inactive, 1 = active
} twt_rtm_t;

extern twt_rtm_t *p_twt;
#endif /* TWT_CONFIG_H_ */
