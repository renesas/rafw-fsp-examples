/***********************************************************************************************************************
* File Name    : otp_example.h
* Description  : Header declarations for OTP initialization and MAC read from OTP.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020-2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#ifndef OTP_EXAMPLE_H
#define OTP_EXAMPLE_H

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "bsp_api.h"
#include "bsp_otp.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

#define OTP_OFFSET_CS_SCRIPT    0x040     /**< CS script region start (words) */

#define OTP_SIZE_CS_SCRIPT      192       /**< 192 words (768 bytes) */

/** Example Data Sizes */
#define OTP_EXAMPLE_MAC_SIZE    6         /**< MAC address size (bytes) */

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/** OTP Example Result Codes */
typedef enum e_otp_example_result
{
    OTP_EXAMPLE_SUCCESS = 0,
    OTP_EXAMPLE_ERROR_INIT_FAILED,
    OTP_EXAMPLE_ERROR_READ_FAILED,
    OTP_EXAMPLE_ERROR_INVALID_OFFSET,
    OTP_EXAMPLE_ERROR_INVALID_SIZE,
} otp_example_result_t;

/***********************************************************************************************************************
 * Exported global functions
 **********************************************************************************************************************/

/**
 * @brief Initialize OTP example
 *
 * Initializes the OTP controller for read operations.
 *
 * @retval OTP_EXAMPLE_SUCCESS          Initialization successful
 * @retval OTP_EXAMPLE_ERROR_INIT_FAILED Initialization failed
 */
otp_example_result_t otp_example_init(void);

/**
 * @brief Read single word from OTP
 *
 * Reads a single 32-bit word from OTP.
 *
 * @param[in] offset   Word offset in OTP
 * @return 32-bit data read from OTP, or 0xFFFFFFFF on error
 */
uint32_t otp_example_read_word(uint32_t offset);

/**
 * @brief Read MAC address from OTP
 *
 * Convenience function to read MAC address from predefined location.
 *
 * @param[out] mac_addr Buffer to store 6-byte MAC address
 *
 * @retval OTP_EXAMPLE_SUCCESS          Read successful
 * @retval OTP_EXAMPLE_ERROR_READ_FAILED Read failed
 */
otp_example_result_t otp_example_read_mac(uint8_t *mac_addr);

/**
 * @brief Run OTP read example demonstration
 *
 * This function demonstrates reading from OTP:
 * 1. Initialize OTP
 * 2. Read sample device data
 * 3. Display results
 *
 * @retval OTP_EXAMPLE_SUCCESS          All operations successful
 * @retval OTP_EXAMPLE_ERROR_*          Error code from failed operation
 */
otp_example_result_t otp_example_run_read_demo(void);

/**
 * @brief Close OTP example
 *
 * Closes the OTP controller and releases resources.
 */
void otp_example_close(void);

#endif /* OTP_EXAMPLE_H */
