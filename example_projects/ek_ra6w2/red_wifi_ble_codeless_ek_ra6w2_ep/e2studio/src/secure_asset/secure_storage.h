/***********************************************************************************************************************

* File Name    : secure_storage.h

* Description  : Secure Asset functions declarations

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates

*

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/

#ifndef RM_SECURE_STORAGE_H
#define RM_SECURE_STORAGE_H

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define SECURE_ASSET_BASE_ID  (1901)

#define SECURE_ASSET_APP_INFO (0)
#define SECURE_ASSET_AT_KEY   (1)

#define SECURE_ASSET_FACTORY_STORAGE_OTP   (0)
#define SECURE_ASSET_FACTORY_STORAGE_FLASH (1)
#define SECURE_ASSET_USER_STORAGE_VEE      (2)

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
typedef struct st_app_info
{
    char ssid[32];
    char password[32];
} ap_info_t;

typedef struct st_key_info
{
    char key[32];
} key_info_t;

/***********************************************************************************************************************
 * Function Prototypes
 **********************************************************************************************************************/
fsp_err_t R_RED_SecureAssetProdStore(uint32_t storage_type, uint32_t asset_id, uint8_t *p_data, uint32_t length);
fsp_err_t R_RED_SecureAssetProdLoad(uint32_t storage_type, uint32_t asset_id, uint8_t *p_data, uint32_t length);
fsp_err_t R_RED_SecureAssetStore(uint32_t asset_id, uint8_t *p_data, uint32_t length);
fsp_err_t R_RED_SecureAssetLoad(uint32_t asset_id, uint8_t *p_data, uint32_t length);
fsp_err_t R_RED_SecureAssetDelete(void);

#endif
