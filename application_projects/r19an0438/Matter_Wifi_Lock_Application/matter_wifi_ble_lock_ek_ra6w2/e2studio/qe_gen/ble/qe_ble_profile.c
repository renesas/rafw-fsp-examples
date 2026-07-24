/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
* this software. By using this software, you agree to the additional terms and conditions found by accessing the
* following link:
* https://www.renesas.com/disclaimer
*
* Copyright (C) 2021 Renesas Electronics Corporation.
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name    : qe_ble_profile.c
* Description  : This file includes definitions.
***********************************************************************************************************************/
/***********************************************************************************************************************
* History      : MM/DD/YYYY Version Description
*              : 08/31/2021 1.00    First Release
***********************************************************************************************************************/

#include "qe_ble_profile.h"

static uint16_t qe_uuid1   = 0x2800;
static uint16_t qe_value1  = 0x1800;
static uint16_t qe_uuid2   = 0x2803;
static uint16_t qe_uuid3   = 0x2A00;
static uint16_t qe_uuid4   = 0x2803;
static uint16_t qe_uuid5   = 0x2A01;
static uint16_t qe_uuid6   = 0x2803;
static uint16_t qe_uuid7   = 0x2A04;
static uint16_t qe_uuid8   = 0x2803;
static uint16_t qe_uuid9   = 0x2AA6;
static uint16_t qe_uuid10  = 0x2803;
static uint16_t qe_uuid11  = 0x2AC9;
static uint16_t qe_uuid12  = 0x2800;
static uint16_t qe_value12 = 0x1801;
static uint16_t qe_uuid13  = 0x2803;
static uint16_t qe_uuid14  = 0x2A05;
static uint16_t qe_uuid15  = 0x2902;
static uint16_t qe_uuid16  = 0x2800;

static uint16_t qe_value16[] = {
  0xF5C1, 0x5CDC, 0x47B3, 0xA3CA, 0x4727, 0x1B4B, 0xB201, 0x9161
};

static uint16_t qe_uuid17  = 0x2803;
static uint16_t qe_uuid19  = 0x2901;
static uint16_t qe_uuid20  = 0x2803;
static uint16_t qe_uuid22  = 0x2902;
static uint16_t qe_uuid23  = 0x2901;
static uint16_t qe_uuid24  = 0x2803;
static uint16_t qe_uuid26  = 0x2803;

static r_ble_gtl_attr_write_cmd_t qe_write_cmd_cb_array[QE_BLE_PROFILE_NUMBER_OF_ATTRIBUTES];

const attribute_t qe_ble_profile[QE_ATTRIBUTE_HANDLE_PROFILE_END] =
{
    // Profile Declaration
#if defined (__SUPPORT_MATTER_IOT__)
   [0] = {
        .handle                  = 0,
        .encapsulated_attributes = 33,
        .permissions             = 0x00,
        .uuid_length             = 0x00,
        .value_length            = 0x00,
        .notify_write            = 0,
        .notify_read             = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[0])
    },
#else
    [0] = {
        .handle                  = 0,
        .encapsulated_attributes = 27,
        .permissions             = 0x00,
        .uuid_length             = 0x00,
        .value_length            = 0x00,
        .notify_write            = 0,
        .notify_read             = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[0])
    },
#endif
    // Service Declaration: GAP Service
    [1] = {
        .handle                  = 1,
        .encapsulated_attributes = 11,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x02,
        .uuid                    = (uint8_t *) &qe_uuid1,
        .value                   = (uint8_t *) &qe_value1,
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[1])
    },

    // Characteristic Declaration: Device Name
    [2] = {
        .handle                  = 2,
        .encapsulated_attributes = 0,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x05,
        .uuid                    = (uint8_t *) &qe_uuid2,
        .value                   = (uint8_t []) { 0x0A, 0x03, 0x00, 0x00, 0x2A },
        .notify_write            = 1,
        .notify_read             = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[2])
    },

    // Characteristic Value: Device Name
    [3] = {
        .handle                  = 3,
        .encapsulated_attributes = 0,
        .permissions             = 0x03,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x80,
        .uuid                    = (uint8_t *) &qe_uuid3,
        .value                   = (uint8_t [0x80]) {},
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[3])
    },

    // Characteristic Declaration: Appearance
    [4] = {
        .handle                  = 4,
        .encapsulated_attributes = 0,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x05,
        .uuid                    = (uint8_t *) &qe_uuid4,
        .value                   = (uint8_t []) { 0x02, 0x05, 0x00, 0x01, 0x2A },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[4])
    },

    // Characteristic Value: Appearance
    [5] = {
        .handle                  = 5,
        .encapsulated_attributes = 0,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x02,
        .uuid                    = (uint8_t *) &qe_uuid5,
        .value                   = (uint8_t []) { 0x00, 0x00 },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[5])
    },

    // Characteristic Declaration: Peripheral Preferred Connection Parameters
    [6] = {
        .handle                  = 6,
        .encapsulated_attributes = 0,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x05,
        .uuid                    = (uint8_t *) &qe_uuid6,
        .value                   = (uint8_t []) { 0x02, 0x07, 0x00, 0x04, 0x2A },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[6])
    },

    // Characteristic Value: Peripheral Preferred Connection Parameters
    [7] = {
        .handle                  = 7,
        .encapsulated_attributes = 0,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x08,
        .uuid                    = (uint8_t *) &qe_uuid7,
        .value                   = (uint8_t []) { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[7])
    },

    // Characteristic Declaration: Central Address Resolution
    [8] = {
        .handle                  = 8,
        .encapsulated_attributes = 0,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x05,
        .uuid                    = (uint8_t *) &qe_uuid8,
        .value                   = (uint8_t []) { 0x02, 0x09, 0x00, 0xA6, 0x2A },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[8])
    },

    // Characteristic Value: Central Address Resolution
    [9] = {
        .handle                  = 9,
        .encapsulated_attributes = 0,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x01,
        .uuid                    = (uint8_t *) &qe_uuid9,
        .value                   = (uint8_t []) { 0x00 },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[9])
    },

    // Characteristic Declaration: Resolvable Private Address Only
    [10] = {
        .handle                  = 10,
        .encapsulated_attributes = 0,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x05,
        .uuid                    = (uint8_t *) &qe_uuid10,
        .value                   = (uint8_t []) { 0x02, 0x0B, 0x00, 0xC9, 0x2A },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[10])
    },

    // Characteristic Value: Resolvable Private Address Only
    [11] = {
        .handle                 = 11,
        .encapsulated_attributes = 0,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x01,
        .uuid                    = (uint8_t *) &qe_uuid11,
        .value                   = (uint8_t []) { 0x00 },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[11])
    },

    // Service Declaration: GATT Service
    [12] = {
        .handle                  = 12,
        .encapsulated_attributes = 4,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x02,
        .uuid                    = (uint8_t *) &qe_uuid12,
        .value                   = (uint8_t *) &qe_value12,
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[12])
    },

    // Characteristic Declaration: Service Changed
    [13] = {
        .handle                  = 13,
        .encapsulated_attributes = 0,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x05,
        .uuid                    = (uint8_t *) &qe_uuid13,
        .value                   = (uint8_t []) { 0x20, 0x0E, 0x00, 0x05, 0x2A },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[13])
    },

    // Characteristic Value: Service Changed
    [14] = {
        .handle                  = 14,
        .encapsulated_attributes = 0,
        .permissions             = 0x00,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x04,
        .uuid                    = (uint8_t *) &qe_uuid14,
        .value                   = (uint8_t []) { 0x00, 0x00, 0x00, 0x00 },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[14])
    },

    // Descriptor: Client Characteristic Configuration
    [15] = {
        .handle                  = 15,
        .encapsulated_attributes = 0,
        .permissions             = 0x03,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x02,
        .uuid                    = (uint8_t *) &qe_uuid15,
        .value                   = (uint8_t []) { 0x00, 0x00 },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 1,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[15])
    },

    // service MATTER
    [16] = {
        .handle                  = 16,
        .encapsulated_attributes = 18,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x02,
        .uuid                    = (uint8_t *) &qe_uuid16,
        .value                   = (uint8_t []) { 0xF6, 0xFF },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[16])
    },

    // Characteristic Declaration: WiFiCommand
    [17] = {
        .handle                  = 17,
        .encapsulated_attributes = 0,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x13,
        .uuid                    = (uint8_t *) &qe_uuid17,
        .value                   = (uint8_t []) { 0x08, 0x12, 0x00, 0xC1, 0xF5, 0xDC, 0x5C, 0xB3, 0x47, 0xCA, 0xA3, 0x27, 0x47, 0x4B, 0x1B, 0x02, 0xB2, 0x61, 0x91 },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[17])
    },

    // Characteristic Value: WiFiCommand
    [18] = {
        .handle                  = 18,
        .encapsulated_attributes = 0,
        .permissions             = 0x02,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_CUSTOM,
        .value_length            = 0xC0,
        .uuid                    = (uint8_t []) { 0xC1, 0xF5, 0xDC, 0x5C, 0xB3, 0x47, 0xCA, 0xA3, 0x27, 0x47, 0x4B, 0x1B, 0x02, 0xB2, 0x61, 0x91 },
        .value                   = (uint8_t [0xC0]) {},
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[18])
    },

    // Descriptor: Characteristic User Description
    [19] = {
        .handle                  = 19,
        .encapsulated_attributes = 0,
        .permissions             = 0x03,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x0A,
        .uuid                    = (uint8_t *) &qe_uuid19,
        .value                   = (uint8_t []) { "Wi-Fi Cmd" },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[19])
    },

    // Characteristic Declaration: WiFiStatus
    [20] = {
        .handle                  = 20,
        .encapsulated_attributes = 0,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x13,
        .uuid                    = (uint8_t *) &qe_uuid20,
        .value                   = (uint8_t []) { 0x12, 0x15, 0x00, 0xC1, 0xF5, 0xDC, 0x5C, 0xB3, 0x47, 0xCA, 0xA3, 0x27, 0x47, 0x4B, 0x1B, 0x03, 0xB2, 0x61, 0x91 },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[20])
    },

    // Characteristic Value: WiFiStatus
    [21] = {
        .handle                  = 21,
        .encapsulated_attributes = 0,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_CUSTOM,
        .value_length            = 0x02,
        .uuid                    = (uint8_t []) { 0xC1, 0xF5, 0xDC, 0x5C, 0xB3, 0x47, 0xCA, 0xA3, 0x27, 0x47, 0x4B, 0x1B, 0x03, 0xB2, 0x61, 0x91 },
        .value                   = (uint8_t []) { 0x00, 0x00 },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[21])
    },

    // Descriptor: Client Characteristic Configuration
    [22] = {
        .handle                  = 22,
        .encapsulated_attributes = 0,
        .permissions             = 0x03,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x02,
        .uuid                    = (uint8_t *) &qe_uuid22,
        .value                   = (uint8_t []) { 0x00, 0x00 },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[22])
    },

    // Descriptor: Characteristic User Description
    [23] = {
        .handle                  = 23,
        .encapsulated_attributes = 0,
        .permissions             = 0x03,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x16,
        .uuid                    = (uint8_t *) &qe_uuid23,
        .value                   = (uint8_t []) { "Wi-Fi Action Result" },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[23])
    },

    // Characteristic Declaration: WiFiOutput
    [24] = {
        .handle                  = 24,
        .encapsulated_attributes = 0,
        .permissions             = 0x03,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x13,
        .uuid                    = (uint8_t *) &qe_uuid24,
        .value                   = (uint8_t []) { 0x03, 0x19, 0x00, 0xC1, 0xF5, 0xDC, 0x5C, 0xB3, 0x47, 0xCA, 0xA3, 0x27, 0x47, 0x4B, 0x1B, 0x04, 0xB2, 0x61, 0x91 },
        .notify_write            = 1,
        .notify_read             = 1,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[24])
    },

    // Characteristic Value: WiFiOutput
    [25] = {
        .handle                  = 25,
        .encapsulated_attributes = 0,
        .permissions             = 0x03,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_CUSTOM,
        .value_length            = 0xC0,
        .uuid                    = (uint8_t []) { 0xC1, 0xF5 ,0xDC, 0x5C, 0xB3, 0x47, 0xCA, 0xA3, 0x27, 0x47, 0x4B, 0x1B, 0x04, 0xB2, 0x61, 0x91 },
        .value                   = (uint8_t [0xC0]) {},
        .notify_write            = 1,
        .notify_read             = 1,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[25])
    },

    // Characteristic Declaration: WiFiProvisoning
    [26] = {
        .handle                  = 26,
        .encapsulated_attributes = 0,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length            = 0x13,
        .uuid                    = (uint8_t *) &qe_uuid26,
        .value                   = (uint8_t []) { 0x02, 0x1B, 0x00, 0xC1, 0xF5, 0xDC, 0x5C, 0xB3, 0x47, 0xCA, 0xA3, 0x27, 0x47, 0x4B, 0x1B, 0x05, 0xB2, 0x61, 0x91 },
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[26])
    },

    // Characteristic Value: WiFiProvisoning
    [27] = {
        .handle                  = 27,
        .encapsulated_attributes = 0,
        .permissions             = 0x01,
        .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_CUSTOM,
        .value_length            = 0xC0,
        .uuid                    = (uint8_t []) { 0xC1, 0xF5, 0xDC, 0x5C, 0xB3, 0x47, 0xCA, 0xA3, 0x27, 0x47, 0x4B, 0x1B, 0x05, 0xB2, 0x61, 0x91 },
        .value                   = (uint8_t [0xC0]) {},
        .notify_write            = 1,
        .notify_read             = 0,
        .read_indication         = 0,
        .p_write_cmd_cb          = &(qe_write_cmd_cb_array[27])

#if defined (__SUPPORT_MATTER_IOT__)
    },
    // add for Matter
        // Charateristic Declairation: Matter RX
        [28] = {
            .handle                  = 28,
            .encapsulated_attributes = 0,
            .permissions             = 0x01,
            .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
            .value_length            = 0,
            .uuid                    = (uint8_t []) { 0x03, 0x28 },
            .value                   = (uint8_t []) { 0x08, 0x1D, 0x00, 0x11, 0x9D, 0x9F, 0x42, 0x9C, 0x4F, 0x9F, 0x95, 0x59, 0x45, 0x3D, 0x26, 0xF5, 0x2E, 0xEE, 0x18 },
            .notify_write            = 1,
            .notify_read             = 0,
            .read_indication         = 0,
            .p_write_cmd_cb          = &(qe_write_cmd_cb_array[28])
        },

        // Charateristic Value: Matter RX
        [29] = {
            .handle                  = 29,
            .encapsulated_attributes = 0,
            .permissions             = 0x02,
            .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_CUSTOM,
            .value_length            = 0xE8,
            .uuid                    = (uint8_t []) { 0x11, 0x9D, 0x9F, 0x42, 0x9C, 0x4F, 0x9F, 0x95, 0x59, 0x45, 0x3D, 0x26, 0xF5, 0x2E, 0xEE, 0x18 },
            .value                   = (uint8_t [0xE8]) { },
            .notify_write            = 1,
            .notify_read             = 0,
            .read_indication         = 0,
            .p_write_cmd_cb          = &(qe_write_cmd_cb_array[29])
        },

        // Charateristic Declairation: Matter TX
        [30] = {
            .handle                  = 30,
            .encapsulated_attributes = 0,
            .permissions             = 0x01,            
            .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
            .value_length            = 0x03,
            .uuid                    = (uint8_t []) { 0x03, 0x28 },
            .value                   = (uint8_t []) { 0x22, 0x1F, 0x00, 0x12, 0x9D, 0x9F, 0x42, 0x9C, 0x4F, 0x9F, 0x95, 0x59, 0x45, 0x3D, 0x26, 0xF5, 0x2E, 0xEE, 0x18 },
            .notify_write            = 1,
            .notify_read             = 0,
            .read_indication         = 0,
            .p_write_cmd_cb          = &(qe_write_cmd_cb_array[30])
        },

        // Charateristic Value: Matter TX
        [31] = {
            .handle                  = 31,
            .encapsulated_attributes = 0,
            .permissions             = 0x21,
            .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_CUSTOM,
            .value_length            = 0xE8,
            .uuid                    = (uint8_t []) { 0x12, 0x9D, 0x9F, 0x42, 0x9C, 0x4F, 0x9F, 0x95, 0x59, 0x45, 0x3D, 0x26, 0xF5, 0x2E, 0xEE, 0x18 },
            .value                   = (uint8_t [0xE8]) { },
            .notify_write            = 1,
            .notify_read             = 1,
            .read_indication         = 1,
            .p_write_cmd_cb          = &(qe_write_cmd_cb_array[31])
        },

        // Descriptor: Characteristic Matter TX Configuration
        [32] = {
            .handle                  = 32,
            .encapsulated_attributes = 0,
            .permissions             = 0x03,            
            .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
            .value_length            = 0x02,
            .uuid                    = (uint8_t []) { 0x02, 0x29 },
            .value                   = (uint8_t []) { 0x03, 0x00 },
            .notify_write            = 1,
            .notify_read             = 0,
            .read_indication         = 0,
            .p_write_cmd_cb          = &(qe_write_cmd_cb_array[32])
        },
        // Descriptor: Characteristic User Description
        [33] = {
            .handle                  = 33,
            .encapsulated_attributes = 0,
            .permissions             = 0x03,
            // .permissions             = 0x22,
            .uuid_length             = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
            .value_length            = 0x0d,
            .uuid                    = (uint8_t []) { 0x01, 0x29 },
            .value                   = (uint8_t []) { "Matter Result" },
            .notify_write            = 1,
            .notify_read             = 0,
            .read_indication         = 0,
            .p_write_cmd_cb          = &(qe_write_cmd_cb_array[33])

#endif // __SUPPORT_MATTER_IOT__
    }
};
