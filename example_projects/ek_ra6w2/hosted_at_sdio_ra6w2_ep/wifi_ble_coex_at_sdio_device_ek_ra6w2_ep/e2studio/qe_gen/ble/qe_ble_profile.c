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
* Copyright (C) 2024 Renesas Electronics Corporation.
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name    : qe_ble_profile.c
* Description  : This file includes definitions.
***********************************************************************************************************************/
/***********************************************************************************************************************
* History      : MM/DD/YYYY Version Description
*              : 10/28/2024 1.00    First Release
***********************************************************************************************************************/

#include "qe_ble_profile.h"

static uint16_t qe_uuid1 = 0x2800;
static uint16_t qe_value1 = 0x1800;
static uint16_t qe_uuid2 = 0x2803;
static uint16_t qe_uuid3 = 0x2A00;
static uint16_t qe_uuid4 = 0x2803;
static uint16_t qe_uuid5 = 0x2A01;
static uint16_t qe_uuid6 = 0x2803;
static uint16_t qe_uuid7 = 0x2A04;
static uint16_t qe_uuid8 = 0x2803;
static uint16_t qe_uuid9 = 0x2AA6;
static uint16_t qe_uuid10 = 0x2803;
static uint16_t qe_uuid11 = 0x2AC9;
static uint16_t qe_uuid12 = 0x2800;
static uint16_t qe_value12 = 0x1801;
static uint16_t qe_uuid13 = 0x2803;
static uint16_t qe_uuid14 = 0x2A05;
static uint16_t qe_uuid15 = 0x2902;
static uint16_t qe_uuid16 = 0x2800;
static uint16_t qe_value16 = 0x180A;
static uint16_t qe_uuid17 = 0x2803;
static uint16_t qe_uuid18 = 0x2A29;
static uint16_t qe_uuid19 = 0x2803;
static uint16_t qe_uuid20 = 0x2A24;
static uint16_t qe_uuid21 = 0x2803;
static uint16_t qe_uuid22 = 0x2A25;
static uint16_t qe_uuid23 = 0x2803;
static uint16_t qe_uuid24 = 0x2A27;
static uint16_t qe_uuid25 = 0x2803;
static uint16_t qe_uuid26 = 0x2A26;
static uint16_t qe_uuid27 = 0x2803;
static uint16_t qe_uuid28 = 0x2A28;
static uint16_t qe_uuid29 = 0x2803;
static uint16_t qe_uuid30 = 0x2A23;
static uint16_t qe_uuid31 = 0x2803;
static uint16_t qe_uuid32 = 0x2A2A;
static uint16_t qe_uuid33 = 0x2803;
static uint16_t qe_uuid34 = 0x2A50;

static r_ble_gtl_attr_write_cmd_t qe_write_cmd_cb_array[QE_BLE_PROFILE_NUMBER_OF_ATTRIBUTES];

const attribute_t qe_ble_profile[QE_ATTRIBUTE_HANDLE_PROFILE_END] =
{

    // Profile Declaration
    [0] =
    {
        .handle = 0,
        .encapsulated_attributes = 34,
        .permissions = 0x00, 
        .uuid_length = 0x00,
        .value_length = 0x00,
        .notify_write = 0,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[0])
    },
    // Service Declaration: GAP Service
    [1] =
    {
        .handle = 1,
        .encapsulated_attributes = 11,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x02,
        .uuid = (uint8_t *)&qe_uuid1,
        .value = (uint8_t *)&qe_value1,
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[1])
    },
    // Characteristic Declaration: Device Name
    [2] =
    {
        .handle = 2,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x05,
        .uuid = (uint8_t *)&qe_uuid2,
        .value = (uint8_t []){0x0A,0x03,0x00,0x00,0x2A},
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[2])
    },
    // Characteristic Value: Device Name
    [3] =
    {
        .handle = 3,
        .encapsulated_attributes = 0,
        .permissions = 0x03,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x80,
        .uuid = (uint8_t *)&qe_uuid3,
        .value = (uint8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[3])
    },
    // Characteristic Declaration: Appearance
    [4] =
    {
        .handle = 4,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x05,
        .uuid = (uint8_t *)&qe_uuid4,
        .value = (uint8_t []){0x02,0x05,0x00,0x01,0x2A},
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[4])
    },
    // Characteristic Value: Appearance
    [5] =
    {
        .handle = 5,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x02,
        .uuid = (uint8_t *)&qe_uuid5,
        .value = (uint8_t []){0x00,0x00},
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[5])
    },
    // Characteristic Declaration: Peripheral Preferred Connection Parameters
    [6] =
    {
        .handle = 6,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x05,
        .uuid = (uint8_t *)&qe_uuid6,
        .value = (uint8_t []){0x02,0x07,0x00,0x04,0x2A},
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[6])
    },
    // Characteristic Value: Peripheral Preferred Connection Parameters
    [7] =
    {
        .handle = 7,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x08,
        .uuid = (uint8_t *)&qe_uuid7,
        .value = (uint8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[7])
    },
    // Characteristic Declaration: Central Address Resolution
    [8] =
    {
        .handle = 8,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x05,
        .uuid = (uint8_t *)&qe_uuid8,
        .value = (uint8_t []){0x02,0x09,0x00,0xA6,0x2A},
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[8])
    },
    // Characteristic Value: Central Address Resolution
    [9] =
    {
        .handle = 9,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x01,
        .uuid = (uint8_t *)&qe_uuid9,
        .value = (uint8_t []){0x00},
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[9])
    },
    // Characteristic Declaration: Resolvable Private Address Only
    [10] =
    {
        .handle = 10,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x05,
        .uuid = (uint8_t *)&qe_uuid10,
        .value = (uint8_t []){0x02,0x0B,0x00,0xC9,0x2A},
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[10])
    },
    // Characteristic Value: Resolvable Private Address Only
    [11] =
    {
        .handle = 11,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x01,
        .uuid = (uint8_t *)&qe_uuid11,
        .value = (uint8_t []){0x00},
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[11])
    },
    // Service Declaration: GATT Service
    [12] =
    {
        .handle = 12,
        .encapsulated_attributes = 4,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x02,
        .uuid = (uint8_t *)&qe_uuid12,
        .value = (uint8_t *)&qe_value12,
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[12])
    },
    // Characteristic Declaration: Service Changed
    [13] =
    {
        .handle = 13,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x05,
        .uuid = (uint8_t *)&qe_uuid13,
        .value = (uint8_t []){0x20,0x0E,0x00,0x05,0x2A},
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[13])
    },
    // Characteristic Value: Service Changed
    [14] =
    {
        .handle = 14,
        .encapsulated_attributes = 0,
        .permissions = 0x00,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x04,
        .uuid = (uint8_t *)&qe_uuid14,
        .value = (uint8_t []){0x00,0x00,0x00,0x00},
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[14])
    },
    // Descriptor: Client Characteristic Configuration
    [15] =
    {
        .handle = 15,
        .encapsulated_attributes = 0,
        .permissions = 0x03,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x02,
        .uuid = (uint8_t *)&qe_uuid15,
        .value = (uint8_t []){0x00,0x00},
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 1,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[15])
    },
    // Service Declaration: Device Information Service2
    [16] =
    {
        .handle = 16,
        .encapsulated_attributes = 19,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x02,
        .uuid = (uint8_t *)&qe_uuid16,
        .value = (uint8_t *)&qe_value16,
        .notify_write = 1,
        .notify_read = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[16])
    },
    // Characteristic Declaration: Manufacturer Name String
    [17] =
    {
        .handle = 17,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x05,
        .uuid = (uint8_t *)&qe_uuid17,
        .value = (uint8_t []){0x0A,0x12,0x00,0x29,0x2A},
        .notify_write = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[17])
    },
    // Characteristic Value: Manufacturer Name String
    [18] =
    {
        .handle = 18,
        .encapsulated_attributes = 0,
        .permissions = 0x03,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x64,
        .uuid = (uint8_t *)&qe_uuid18,
        .value = (uint8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        .notify_write = 1,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[18])
    },
    // Characteristic Declaration: Model Number String
    [19] =
    {
        .handle = 19,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x05,
        .uuid = (uint8_t *)&qe_uuid19,
        .value = (uint8_t []){0x02,0x14,0x00,0x24,0x2A},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[19])
    },
    // Characteristic Value: Model Number String
    [20] =
    {
        .handle = 20,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x64,
        .uuid = (uint8_t *)&qe_uuid20,
        .value = (uint8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[20])
    },
    // Characteristic Declaration: Serial Number String
    [21] =
    {
        .handle = 21,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x05,
        .uuid = (uint8_t *)&qe_uuid21,
        .value = (uint8_t []){0x02,0x16,0x00,0x25,0x2A},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[21])
    },
    // Characteristic Value: Serial Number String
    [22] =
    {
        .handle = 22,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x64,
        .uuid = (uint8_t *)&qe_uuid22,
        .value = (uint8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[22])
    },
    // Characteristic Declaration: Hardware Revision String
    [23] =
    {
        .handle = 23,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x05,
        .uuid = (uint8_t *)&qe_uuid23,
        .value = (uint8_t []){0x02,0x18,0x00,0x27,0x2A},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[23])
    },
    // Characteristic Value: Hardware Revision String
    [24] =
    {
        .handle = 24,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x64,
        .uuid = (uint8_t *)&qe_uuid24,
        .value = (uint8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[24])
    },
    // Characteristic Declaration: Firmware Revision String
    [25] =
    {
        .handle = 25,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x05,
        .uuid = (uint8_t *)&qe_uuid25,
        .value = (uint8_t []){0x02,0x1A,0x00,0x26,0x2A},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[25])
    },
    // Characteristic Value: Firmware Revision String
    [26] =
    {
        .handle = 26,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x64,
        .uuid = (uint8_t *)&qe_uuid26,
        .value = (uint8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[26])
    },
    // Characteristic Declaration: Software Revision String
    [27] =
    {
        .handle = 27,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x05,
        .uuid = (uint8_t *)&qe_uuid27,
        .value = (uint8_t []){0x02,0x1C,0x00,0x28,0x2A},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[27])
    },
    // Characteristic Value: Software Revision String
    [28] =
    {
        .handle = 28,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x64,
        .uuid = (uint8_t *)&qe_uuid28,
        .value = (uint8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[28])
    },
    // Characteristic Declaration: System ID
    [29] =
    {
        .handle = 29,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x05,
        .uuid = (uint8_t *)&qe_uuid29,
        .value = (uint8_t []){0x02,0x1E,0x00,0x23,0x2A},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[29])
    },
    // Characteristic Value: System ID
    [30] =
    {
        .handle = 30,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x08,
        .uuid = (uint8_t *)&qe_uuid30,
        .value = (uint8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[30])
    },
    // Characteristic Declaration: IEEE 11073-20601 Regulatory Certification Data List
    [31] =
    {
        .handle = 31,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x05,
        .uuid = (uint8_t *)&qe_uuid31,
        .value = (uint8_t []){0x02,0x20,0x00,0x2A,0x2A},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[31])
    },
    // Characteristic Value: IEEE 11073-20601 Regulatory Certification Data List
    [32] =
    {
        .handle = 32,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x64,
        .uuid = (uint8_t *)&qe_uuid32,
        .value = (uint8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[32])
    },
    // Characteristic Declaration: PnP ID
    [33] =
    {
        .handle = 33,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x05,
        .uuid = (uint8_t *)&qe_uuid33,
        .value = (uint8_t []){0x02,0x22,0x00,0x50,0x2A},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[33])
    },
    // Characteristic Value: PnP ID
    [34] =
    {
        .handle = 34,
        .encapsulated_attributes = 0,
        .permissions = 0x01,
        .uuid_length = QE_BLE_PROFILE_UUID_SIZE_ADOPTED,
        .value_length = 0x07,
        .uuid = (uint8_t *)&qe_uuid34,
        .value = (uint8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        .notify_write = 1,
        .notify_read = 0,
        .read_indication = 0,
        .p_write_cmd_cb = &(qe_write_cmd_cb_array[34])
    }

};
