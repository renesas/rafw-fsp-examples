/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
 *    All rights reserved.
 *    Copyright (c) 2023 Modified by Renesas Electronics Corporation
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Example project configuration file for CHIP.
 *
 *          This is a place to put application or project-specific overrides
 *          to the default configuration values for general CHIP features.
 *
 */

#pragma once

/* Use a default pairing code if one hasn't been provisioned in flash.*/
#ifndef CHIP_DEVICE_CONFIG_USE_TEST_SETUP_PIN_CODE
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_PIN_CODE 20202021
#endif //CHIP_DEVICE_CONFIG_USE_TEST_SETUP_PIN_CODE

#ifndef CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR 0xF00
#endif //CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR

/* For convenience, Chip Security Test Mode can be enabled and the
requirement for authentication in various protocols can be disabled.
WARNING: These options make it possible to circumvent basic Chip security functionality,
including message encryption. Because of this they MUST NEVER BE ENABLED IN PRODUCTION BUILDS.
*/
#ifndef CHIP_CONFIG_SECURITY_TEST_MODE
#define CHIP_CONFIG_SECURITY_TEST_MODE 0
#endif //CHIP_CONFIG_SECURITY_TEST_MODE

/**
 * CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID
 *
 * 0xFFF1: Test vendor
 */
#ifndef CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID
#define CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID 0xFFF1
#endif //CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID

/**
 * CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID
 *
 * 0x8006: example lock app
 */
#ifndef CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID
#define CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID 0x8006
#endif //CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID

/**
 * CHIP_DEVICE_CONFIG_DEVICE_HARDWARE_VERSION
 *
 * The hardware version number assigned to device or product by the device vendor.  This
 * number is scoped to the device product id, and typically corresponds to a revision of the
 * physical device, a change to its packaging, and/or a change to its marketing presentation.
 * This value is generally *not* incremented for device software versions.
 */
#ifndef CHIP_DEVICE_CONFIG_DEVICE_HARDWARE_VERSION
#define CHIP_DEVICE_CONFIG_DEVICE_HARDWARE_VERSION 1
#endif //CHIP_DEVICE_CONFIG_DEVICE_HARDWARE_VERSION

/**
 * CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION
 *
 * A uint32_t identifying the software version running on the device.
 */
/* The SoftwareVersion attribute of the Basic cluster. */
#ifndef CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION
#define CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION 0x0001
#endif

/**
 * CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
 *
 * Enable support for Chip-over-BLE (CHIPoBLE).
 */
#ifdef __RRQ61400__
#ifndef CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
#define CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE 1
#endif //CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE

#else
#ifndef CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
#define CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE 0
#endif //CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
#endif //__RRQ61400__

/**
 * CHIP_DEVICE_CONFIG_TEST_SERIAL_NUMBER
 *
 * Enables the use of a hard-coded default serial number if none
 * is found in Chip NV storage.
 */
#ifndef CHIP_DEVICE_CONFIG_TEST_SERIAL_NUMBER
#define CHIP_DEVICE_CONFIG_TEST_SERIAL_NUMBER "TEST_SN"
#endif //CHIP_DEVICE_CONFIG_TEST_SERIAL_NUMBER

/**
 * CHIP_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS
 *
 * Enable recording UTC timestamps.
 */
#ifndef CHIP_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS
#define CHIP_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS 0
#endif //CHIP_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS

/**
 * CHIP_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_BUFFER_SIZE
 *
 * A size, in bytes, of the individual debug event logging buffer.
 */
#ifndef CHIP_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_BUFFER_SIZE
#define CHIP_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_BUFFER_SIZE (512)
#endif //CHIP_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_BUFFER_SIZE

/**
 *  @def CHIP_CONFIG_MRP_LOCAL_ACTIVE_RETRY_INTERVAL
 *
 *  @brief
 *    Active retransmit interval, or time to wait before retransmission after
 *    subsequent failures in milliseconds.
 *
 *  This is the default value, that might be adjusted by end device depending on its
 *  needs (e.g. sleeping period) using Service Discovery TXT record CRA key.
 *
 */
#ifndef CHIP_CONFIG_MRP_LOCAL_ACTIVE_RETRY_INTERVAL
#define CHIP_CONFIG_MRP_LOCAL_ACTIVE_RETRY_INTERVAL (2000_ms32)
#endif //CHIP_CONFIG_MRP_LOCAL_ACTIVE_RETRY_INTERVAL

#ifndef CHIP_DEVICE_CONFIG_TEST_PART_NUMBER
#define CHIP_DEVICE_CONFIG_TEST_PART_NUMBER		"RA6W1/RA6W2"
#endif //CHIP_DEVICE_CONFIG_TEST_PART_NUMBER

#ifndef CHIP_DEVICE_CONFIG_TEST_PRODUCT_URL
#define CHIP_DEVICE_CONFIG_TEST_PRODUCT_URL		"www.renesas.com"
#endif //CHIP_DEVICE_CONFIG_TEST_PRODUCT_URL

#ifndef CHIP_DEVICE_CONFIG_TEST_PRODUCT_LABLE
#define CHIP_DEVICE_CONFIG_TEST_PRODUCT_LABLE	"RenesasProd1234"
#endif //CHIP_DEVICE_CONFIG_TEST_PRODUCT_LABLE
