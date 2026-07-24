/***********************************************************************************************************************
* File Name    : at_cmd.c
* Description  : AT command processing functions.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "strings.h"
#include "stdarg.h"


#include "sdio_cmd.h"
#include "at_cmd.h"
#if (SUPPORT_MATTER_APP == 1)
#include "matter.h"
#endif // SUPPORT_MATTER_APP

#include "usb_console_main.h"

#define DEBUGGING
#define MAX_RETRY_SEND_COUNT    10
#define IMAGE_CRC_BLOCK_CHECK
#define IMAGE_CRC_IMG_CHECK
#define IMAGE_PATTERN_CHECK
#define SUPPORT_CHECK_CRC_16
#ifdef SUPPORT_CHECK_CRC_16
#define swap16(x) ((uint16_t)(((x) & 0xff) << 8) | (uint16_t)(((x) & 0xff00) >> 8))
static const uint16_t RO_fast_crc_nbit_LUT[4][16] = { {swap16(0x0000), swap16(0x3331), swap16(0x6662), swap16(0x5553), swap16(
    0xccc4), swap16(0xfff5), swap16(0xaaa6), swap16(0x9997), swap16(0x89a9), swap16(0xba98), swap16(0xefcb), swap16(0xdcfa),
    swap16(0x456d), swap16(0x765c), swap16(0x230f), swap16(0x103e)}, {swap16(0x0000), swap16(0x0373), swap16(0x06e6), swap16(
    0x0595), swap16(0x0dcc), swap16(0x0ebf), swap16(0x0b2a), swap16(0x0859), swap16(0x1b98), swap16(0x18eb), swap16(0x1d7e),
    swap16(0x1e0d), swap16(0x1654), swap16(0x1527), swap16(0x10b2), swap16(0x13c1)}, {swap16(0x0000), swap16(0x1021), swap16(
    0x2042), swap16(0x3063), swap16(0x4084), swap16(0x50a5), swap16(0x60c6), swap16(0x70e7), swap16(0x8108), swap16(0x9129),
    swap16(0xa14a), swap16(0xb16b), swap16(0xc18c), swap16(0xd1ad), swap16(0xe1ce), swap16(0xf1ef)}, {swap16(0x0000), swap16(
    0x1231), swap16(0x2462), swap16(0x3653), swap16(0x48c4), swap16(0x5af5), swap16(0x6ca6), swap16(0x7e97), swap16(0x9188),
    swap16(0x83b9), swap16(0xb5ea), swap16(0xa7db), swap16(0xd94c), swap16(0xcb7d), swap16(0xfd2e), swap16(0xef1f)}};
#endif /* SUPPORT_CHECK_CRC_16 */

/* Platform features, configurations, and certification keys */
#if (SUPPORT_SPI == 1) || (SUPPORT_SDIO == 1)
const char *cmd_set_aws_cfg[MAX_CFG_NUM] = {
#if (AWS_USE_FLEET_PROV == 1)
    "AT+"PLATFORM" SET AWS_USE_FP 1",
#else
    "AT+"PLATFORM" SET AWS_USE_FP 0",
#endif
    "AT+"PLATFORM" SET APP_BOARD_FEATURE EVK",
#if (AWS_USE_FLEET_PROV == 0)
    "AT+"PLATFORM" SET APP_THINGNAME enter_thing_name",
#endif
    "AT+"PLATFORM" SET AWS_BROKER enter_broker_url",
    "AT+"PLATFORM" SET APP_LPORT 1883",
    "AT+"PLATFORM" SET APP_SUBTOPIC /AppControl",
    "AT+"PLATFORM" SET APP_PUBTOPIC /DeviceControl",
    "AT+"PLATFORM" CFG 0 app_door 1 2", /* mcu sub. str */
    "AT+"PLATFORM" CFG 1 mcu_door 1 0", /* mcu pub. str */
    "AT+"PLATFORM" CFG 2 app_window 1 2", /* mcu sub. str */
    "AT+"PLATFORM" CFG 3 mcu_window 1 0", /* mcu pub. str */
    "AT+"PLATFORM" CFG 4 battery 0 1", /* shadow int */
    "AT+"PLATFORM" CFG 5 temperature 2 1", /* shadow float */
    "AT+"PLATFORM" CFG 6 doorStat 1 1", /* shadow str */
    "AT+"PLATFORM" CFG 7 windowStat 1 1", /* shadow str */
    "AT+"PLATFORM" CFG 8 app_shadow 1 2", /* mcu sub. str */
    "AT+"PLATFORM" CFG 9 mcu_shadow 1 0", /* mcu pub. str */
    "AT+"PLATFORM" SET SLEEP_MODE 3",
    "AT+"PLATFORM" SET USE_DPM 1",
    "AT+"PLATFORM" SET RTC_TIME 1740",
    "AT+"PLATFORM" SET DPM_KEEP_ALIVE 30000",
    "AT+"PLATFORM" SET USE_WAKE_UP 0",
    "AT+"PLATFORM" SET TIM_WAKE_UP 10",
    "AT+"PLATFORM" SET APP_MCU_WKAEUP_PORT GPIO_UNIT_A", /* GPIO_UNIT_A or GPIO_UNIT_C */
    "AT+"PLATFORM" SET APP_MCU_WKAEUP_PIN GPIO_PIN11", /* GPIO_PIN0 ~ GPIO_PIN11 or GPIO_PIN6~GPIO_PIN8 */
};

const char *cmd_set_azure_cfg[MAX_CFG_NUM] = {"AT+"PLATFORM" SET APP_BOARD_FEATURE EVK",
    "AT+"PLATFORM" SET APP_THINGNAME enter_thing_name",
    "AT+"PLATFORM" SET APP_DEV_PRIMARY_KEY enter_primary_key",
    "AT+"PLATFORM" SET APP_HOSTNAME enter_host_name",
    "AT+"PLATFORM" SET APP_IOTHUB_CONN_STRING enter_connection_string",
    "AT+"PLATFORM" CFG 0 app_door 1 2", /* mcu sub. str */
    "AT+"PLATFORM" CFG 1 mcu_door 1 0", /* mcu pub. str */
    "AT+"PLATFORM" CFG 2 battery 0 1", /* shadow   int */
    "AT+"PLATFORM" CFG 3 temperature 2 1", /* shadow   float */
    "AT+"PLATFORM" CFG 4 doorState 1 1", /* shadow   str */
    "AT+"PLATFORM" CFG 5 app_shadow 1 2", /* mcu sub. str */
    "AT+"PLATFORM" CFG 6 mcu_shadow 1 0", /* mcu pub. str */
    "AT+"PLATFORM" CFG 7 openMethod 1 1", /* shadow   str */
    "AT+"PLATFORM" SET SLEEP_MODE 3", "AT+"PLATFORM" SET USE_DPM 1", "AT+"PLATFORM" SET RTC_TIME 1740", /* 1740 */
    "AT+"PLATFORM" SET DPM_KEEP_ALIVE 30000", "AT+"PLATFORM" SET USE_WAKE_UP 0",
    "AT+"PLATFORM" SET TIM_WAKE_UP 10", "AT+"PLATFORM" SET APP_MCU_WKAEUP_PORT GPIO_UNIT_A", /* GPIO_UNIT_A or GPIO_UNIT_C */
    "AT+"PLATFORM" SET APP_MCU_WKAEUP_PIN GPIO_PIN11" /* GPIO_PIN0 ~ GPIO_PIN11 or GPIO_PIN6~GPIO_PIN8 */
};

char cert_ca1[] = "EC0,-----BEGIN CERTIFICATE-----\n"
    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
    "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
    "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
    "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
    "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
    "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
    "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
    "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
    "-----END CERTIFICATE-----\n";

char cert_cert1[] = "EC1,-----BEGIN CERTIFICATE-----\n"
    "MIIDWjCCAkKgAwIBAgIVAIqSKvd/Qq2E9ZleQWN2Gk/iPw2GMA0GCSqGSIb3DQEB\n"
    "CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\n"
    "IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0xODEyMDYwNjQw\n"
    "MjZaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\n"
    "dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDZ/AbN7xxXgAslyB14\n"
    "ZHV/MPUjrpgPSnrbHcLwhOpKILoHiLO6CTqfXv/pxcXyh0UCHpp1PF63m0vmYYuA\n"
    "ueRgW23sjKPXRPyGnFPVjGntNhlFuXAWX1+m09GLkqdxWGz2wgKokSa8pMO/otTA\n"
    "iV5+uh8y/7q5fuASGywZR5WeutH8yjw4ui5Il+66S2yUifsCDrNgsmfTIZdta4cV\n"
    "umRKG6ZMT7XHEVuFMIrE5N2nfXu62CUWn4GOmmoF4iH0w6FINmV0f0sQcLS73+tv\n"
    "CR/dFnNzRT3DLm8FJH7RD60jHiYoZQFNHWuSH98cAg3RSyuRnHx9mmD+O8jKyZ6D\n"
    "r/7NAgMBAAGjYDBeMB8GA1UdIwQYMBaAFL6HYMtZyM54cz3RAAzyR1zF7+1TMB0G\n"
    "A1UdDgQWBBQeh5c1lEyK80j/TBBMP6Cz/qU3ljAMBgNVHRMBAf8EAjAAMA4GA1Ud\n"
    "DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAp9tbJ4GOFoX11trWKc/HTfdb\n"
    "TMIVu8KeEnIdFadgGhVcafH6cIrVBcocR5iAQNhV28P5dSFSrsdDOaiYQQ6XyaS9\n"
    "oOfLJCHosFd0CCAkV+2ZEmxDA0bN+WDdCppQHocYoNt8h6X+Mh0h2hnfB2hPQwDX\n"
    "TcaCwbJQy2XprqPpBo3ZuWqmSi55uslXj+2B4XgPZutim++8J7DHQbfHAGZwiAFN\n"
    "90TNlhZBdI87Ga07p0db03KcBQs8dBMaABC0RK39LqJ5ZdQMT/Owx0+iO2Be7w30\n"
    "7o06zCQB2A0nmfvAR8gSuImIBfKz2I1xQX5+CO4wes8RH5pNIOK2QrKgr9NJkA==\n"
    "-----END CERTIFICATE-----\n";

char cert_key1[] = "EC2,-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEpAIBAAKCAQEA2fwGze8cV4ALJcgdeGR1fzD1I66YD0p62x3C8ITqSiC6B4iz\n"
    "ugk6n17/6cXF8odFAh6adTxet5tL5mGLgLnkYFtt7Iyj10T8hpxT1Yxp7TYZRblw\n"
    "Fl9fptPRi5KncVhs9sICqJEmvKTDv6LUwIlefrofMv+6uX7gEhssGUeVnrrR/Mo8\n"
    "OLouSJfuuktslIn7Ag6zYLJn0yGXbWuHFbpkShumTE+1xxFbhTCKxOTdp317utgl\n"
    "Fp+BjppqBeIh9MOhSDZldH9LEHC0u9/rbwkf3RZzc0U9wy5vBSR+0Q+tIx4mKGUB\n"
    "TR1rkh/fHAIN0UsrkZx8fZpg/jvIysmeg6/+zQIDAQABAoIBADfE6fy/4xFj2fZF\n"
    "l3yYvxLWdLE3VwH6fSoYGCqu5r4mV1HcIJdFCzGA/ZpSlg0xnG8pYz0BP/5bhfSg\n"
    "Gi/J32rjmWD+rmBB7xWFY1FsRiGBSL/07H9c0Tz+TksWLy6pf981zbZQxIdY5Bfg\n"
    "UewceQeVGKxUjvIsSql3ODYTgW0FR7h+YGtmtXJ+8SQi3FSRwDdbpyoLokUf1YaH\n"
    "ksG1RPOxxah7Jr0YFN4waSixMMSb/fAxF5F1/mD0tgUSkUptRXu879mpA5+uYD+Q\n"
    "YrPzEDhvd8mXPaH1f1e/29Kq+tUNtmBdzY8gcmWr2h859x3R6wpybbYJt5KWK4NT\n"
    "7auoPKECgYEA/7yOGf8y2QJLfjW08qBAATnYZuZySrV7rNK5kxenGueZl2t52NCa\n"
    "vRQC8nNqouu33RjiYHSR8NQk9cLdpjnQOVxtSEWTZIctOPhtw53EAdRfw4v2e/7n\n"
    "oe9kR3VH1OUKfMDhduMUI09UGGyxsyRcKs1uLvvC5DX2XRAGafN1aDkCgYEA2jWD\n"
    "5SAPPJU+cbkQbkSBmcJph8x0949c/HJ2U6xcMmwR8G4Jnlwe+w3expwKnNlpNXaZ\n"
    "I+mm2BeXvyPJCgN/BMkDhU1xyDQBscCYrD9q41IU318CTmH6iExoUwv6NNuyOsFd\n"
    "IJeYnG6ckgI7yGkY0wxQvsI8alleI2mLehHuwzUCgYEA3Ye1xPlXT7r4MH1PoPmG\n"
    "WEmGlyS7DtKFLuFf1fagT+MeHpgAdfvGf1HNd77ZOgZdQI6k0w9HuMnctnO2U58z\n"
    "K+1P0VJL6sJaP0actt58g2U4C4m73A+lEZbxVCFZNyetXQIsjTMKJ8g5PesyR8+Q\n"
    "c5d/Af4fBldkcZtHIxK9uqkCgYAaqpmQwac7Bx4Xdb9NSm/wI3MUFmdg7ZM2gqJ1\n"
    "PUYTH2Pd1wSz5pweoCZObTlay7LwxqqWWfJ6y/9Oa4ghAiZeplYYz0sNZVWjrF68\n"
    "BhAA8cH9PjYg8BZW28eQBpGwLf0M8x53Yi9TRq05pq45oqZW/FVNypzpfjxj5X0X\n"
    "EOP11QKBgQCDnAVbfrXC+4S5UNwxGHw4cZJwAvOkkeApV3WlBSZFbbGzIxrVy79O\n"
    "7ETTGfSAbksUljV+2HZZVSXtgsCS/fzsFjMWYpeNRX3+9wtFfGCfxoygGW0JvOyY\n"
    "kg61geirHUDYgog9XzGKATXc3K/m7JdyOcWdbf54nhzcEqjRv1DhCA==\n"
    "-----END RSA PRIVATE KEY-----\n";

char azure_root_ca[] = "EC0,-----BEGIN CERTIFICATE-----\r\n"
    "MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\r\n"
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\r\n"
    "MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\r\n"
    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n"
    "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\r\n"
    "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\r\n"
    "2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\r\n"
    "1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\r\n"
    "q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\r\n"
    "tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\r\n"
    "vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\r\n"
    "BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\r\n"
    "5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\r\n"
    "1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\r\n"
    "NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\r\n"
    "Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\r\n"
    "8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\r\n"
    "pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\r\n"
    "MrY=\r\n"
    "-----END CERTIFICATE-----\r\n"
    "-----BEGIN CERTIFICATE-----\r\n"
    "MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\r\n"
    "RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\r\n"
    "VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\r\n"
    "DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\r\n"
    "ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\r\n"
    "VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\r\n"
    "mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\r\n"
    "IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\r\n"
    "mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\r\n"
    "XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\r\n"
    "dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\r\n"
    "jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\r\n"
    "BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\r\n"
    "DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\r\n"
    "9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\r\n"
    "jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\r\n"
    "Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\r\n"
    "ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\r\n"
    "R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\r\n"
    "-----END CERTIFICATE-----\r\n";

#else
const char *cmd_set_aws_cfg[MAX_CFG_NUM] = {
#if (AWS_USE_FLEET_PROV == 1)
    "\r\nAT+"PLATFORM" SET AWS_USE_FP 1\r\n",
#else
    "\r\nAT+"PLATFORM" SET AWS_USE_FP 0\r\n",
#endif
    "\r\nAT+"PLATFORM" SET APP_BOARD_FEATURE EVK\r\n",
#if (AWS_USE_FLEET_PROV == 0)
    "\r\nAT+"PLATFORM" SET APP_THINGNAME enter_thing_name\r\n",
#endif
    "\r\nAT+"PLATFORM" SET AWS_BROKER enter_broker_url\r\n",
    "\r\nAT+"PLATFORM" SET APP_LPORT 1883\r\n", "\r\nAT+"PLATFORM" SET APP_SUBTOPIC /AppControl\r\n",
    "\r\nAT+"PLATFORM" SET APP_PUBTOPIC /DeviceControl\r\n", "\r\nAT+"PLATFORM" CFG 0 app_door 1 2\r\n", /* mcu sub. str */
    "\r\nAT+"PLATFORM" CFG 1 mcu_door 1 0\r\n", /* mcu pub. str */
    "\r\nAT+"PLATFORM" CFG 2 app_window 1 2\r\n", /* mcu sub. str */
    "\r\nAT+"PLATFORM" CFG 3 mcu_window 1 0\r\n", /* mcu pub. str */
    "\r\nAT+"PLATFORM" CFG 4 battery 0 1\r\n", /* shadow int */
    "\r\nAT+"PLATFORM" CFG 5 temperature 2 1\r\n", /* shadow float */
    "\r\nAT+"PLATFORM" CFG 6 doorStat 1 1\r\n", /* shadow str */
    "\r\nAT+"PLATFORM" CFG 7 windowStat 1 1\r\n", /* shadow str */
    "\r\nAT+"PLATFORM" CFG 8 app_shadow 1 2\r\n", /* mcu sub. str */
    "\r\nAT+"PLATFORM" CFG 9 mcu_shadow 1 0\r\n", /* mcu pub. str */
    "\r\nAT+"PLATFORM" SET SLEEP_MODE 3\r\n", "\r\nAT+"PLATFORM" SET USE_DPM 1\r\n", "\r\nAT+"PLATFORM" SET RTC_TIME 1740\r\n",
    "\r\nAT+"PLATFORM" SET DPM_KEEP_ALIVE 30000\r\n", "\r\nAT+"PLATFORM" SET USE_WAKE_UP 0\r\n",
    "\r\nAT+"PLATFORM" SET TIM_WAKE_UP 10\r\n", "\r\nAT+"PLATFORM" SET APP_MCU_WKAEUP_PORT GPIO_UNIT_A\r\n", /* GPIO_UNIT_A or GPIO_UNIT_C */
    "\r\nAT+"PLATFORM" SET APP_MCU_WKAEUP_PIN GPIO_PIN11\r\n", /* GPIO_PIN0 ~ GPIO_PIN11 or GPIO_PIN6~GPIO_PIN8 */
};

const char *cmd_set_azure_cfg[MAX_CFG_NUM] = {"\r\nAT+"PLATFORM" SET APP_BOARD_FEATURE EVK\r\n",
    "\r\nAT+"PLATFORM" SET APP_THINGNAME enter_thing_name\r\n",
    "\r\nAT+"PLATFORM" SET APP_DEV_PRIMARY_KEY enter_primary_key\r\n",
    "\r\nAT+"PLATFORM" SET APP_HOSTNAME enter_host_name\r\n",
    "\r\nAT+"PLATFORM" SET APP_IOTHUB_CONN_STRING enter_connection_string\r\n",
    "\r\nAT+"PLATFORM" CFG 0 app_door 1 2\r\n", /* mcu sub. str */
    "\r\nAT+"PLATFORM" CFG 1 mcu_door 1 0\r\n", /* mcu pub. str */
    "\r\nAT+"PLATFORM" CFG 2 battery 0 1\r\n", /* shadow   int */
    "\r\nAT+"PLATFORM" CFG 3 temperature 2 1\r\n", /* shadow   float */
    "\r\nAT+"PLATFORM" CFG 4 doorState 1 1\r\n", /* shadow   str */
    "\r\nAT+"PLATFORM" CFG 5 app_shadow 1 2\r\n", /* mcu sub. str */
    "\r\nAT+"PLATFORM" CFG 6 mcu_shadow 1 0\r\n", /* mcu pub. str */
    "\r\nAT+"PLATFORM" CFG 7 openMethod 1 1\r\n", /* shadow   str */
    "\r\nAT+"PLATFORM" SET SLEEP_MODE 3\r\n", "\r\nAT+"PLATFORM" SET USE_DPM 1\r\n", "\r\nAT+"PLATFORM" SET RTC_TIME 1740\r\n", /* 1740 */
    "\r\nAT+"PLATFORM" SET DPM_KEEP_ALIVE 30000\r\n", "\r\nAT+"PLATFORM" SET USE_WAKE_UP 0\r\n",
    "\r\nAT+"PLATFORM" SET TIM_WAKE_UP 10\r\n", "\r\nAT+"PLATFORM" SET APP_MCU_WKAEUP_PORT GPIO_UNIT_A\r\n", /* GPIO_UNIT_A or GPIO_UNIT_C */
    "\r\nAT+"PLATFORM" SET APP_MCU_WKAEUP_PIN GPIO_PIN11\r\n" /* GPIO_PIN0 ~ GPIO_PIN11 or GPIO_PIN6~GPIO_PIN8 */
};

const char *cert_ca1 = "C0,-----BEGIN CERTIFICATE-----\n"
    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
    "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
    "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
    "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
    "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
    "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
    "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
    "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
    "-----END CERTIFICATE-----\n";

static const char *cert_cert1 = "C1,-----BEGIN CERTIFICATE-----\n"
    "MIIDWjCCAkKgAwIBAgIVAIqSKvd/Qq2E9ZleQWN2Gk/iPw2GMA0GCSqGSIb3DQEB\n"
    "CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\n"
    "IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0xODEyMDYwNjQw\n"
    "MjZaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\n"
    "dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDZ/AbN7xxXgAslyB14\n"
    "ZHV/MPUjrpgPSnrbHcLwhOpKILoHiLO6CTqfXv/pxcXyh0UCHpp1PF63m0vmYYuA\n"
    "ueRgW23sjKPXRPyGnFPVjGntNhlFuXAWX1+m09GLkqdxWGz2wgKokSa8pMO/otTA\n"
    "iV5+uh8y/7q5fuASGywZR5WeutH8yjw4ui5Il+66S2yUifsCDrNgsmfTIZdta4cV\n"
    "umRKG6ZMT7XHEVuFMIrE5N2nfXu62CUWn4GOmmoF4iH0w6FINmV0f0sQcLS73+tv\n"
    "CR/dFnNzRT3DLm8FJH7RD60jHiYoZQFNHWuSH98cAg3RSyuRnHx9mmD+O8jKyZ6D\n"
    "r/7NAgMBAAGjYDBeMB8GA1UdIwQYMBaAFL6HYMtZyM54cz3RAAzyR1zF7+1TMB0G\n"
    "A1UdDgQWBBQeh5c1lEyK80j/TBBMP6Cz/qU3ljAMBgNVHRMBAf8EAjAAMA4GA1Ud\n"
    "DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAp9tbJ4GOFoX11trWKc/HTfdb\n"
    "TMIVu8KeEnIdFadgGhVcafH6cIrVBcocR5iAQNhV28P5dSFSrsdDOaiYQQ6XyaS9\n"
    "oOfLJCHosFd0CCAkV+2ZEmxDA0bN+WDdCppQHocYoNt8h6X+Mh0h2hnfB2hPQwDX\n"
    "TcaCwbJQy2XprqPpBo3ZuWqmSi55uslXj+2B4XgPZutim++8J7DHQbfHAGZwiAFN\n"
    "90TNlhZBdI87Ga07p0db03KcBQs8dBMaABC0RK39LqJ5ZdQMT/Owx0+iO2Be7w30\n"
    "7o06zCQB2A0nmfvAR8gSuImIBfKz2I1xQX5+CO4wes8RH5pNIOK2QrKgr9NJkA==\n"
    "-----END CERTIFICATE-----\n";

const char *cert_key1 = "C2,-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEpAIBAAKCAQEA2fwGze8cV4ALJcgdeGR1fzD1I66YD0p62x3C8ITqSiC6B4iz\n"
    "ugk6n17/6cXF8odFAh6adTxet5tL5mGLgLnkYFtt7Iyj10T8hpxT1Yxp7TYZRblw\n"
    "Fl9fptPRi5KncVhs9sICqJEmvKTDv6LUwIlefrofMv+6uX7gEhssGUeVnrrR/Mo8\n"
    "OLouSJfuuktslIn7Ag6zYLJn0yGXbWuHFbpkShumTE+1xxFbhTCKxOTdp317utgl\n"
    "Fp+BjppqBeIh9MOhSDZldH9LEHC0u9/rbwkf3RZzc0U9wy5vBSR+0Q+tIx4mKGUB\n"
    "TR1rkh/fHAIN0UsrkZx8fZpg/jvIysmeg6/+zQIDAQABAoIBADfE6fy/4xFj2fZF\n"
    "l3yYvxLWdLE3VwH6fSoYGCqu5r4mV1HcIJdFCzGA/ZpSlg0xnG8pYz0BP/5bhfSg\n"
    "Gi/J32rjmWD+rmBB7xWFY1FsRiGBSL/07H9c0Tz+TksWLy6pf981zbZQxIdY5Bfg\n"
    "UewceQeVGKxUjvIsSql3ODYTgW0FR7h+YGtmtXJ+8SQi3FSRwDdbpyoLokUf1YaH\n"
    "ksG1RPOxxah7Jr0YFN4waSixMMSb/fAxF5F1/mD0tgUSkUptRXu879mpA5+uYD+Q\n"
    "YrPzEDhvd8mXPaH1f1e/29Kq+tUNtmBdzY8gcmWr2h859x3R6wpybbYJt5KWK4NT\n"
    "7auoPKECgYEA/7yOGf8y2QJLfjW08qBAATnYZuZySrV7rNK5kxenGueZl2t52NCa\n"
    "vRQC8nNqouu33RjiYHSR8NQk9cLdpjnQOVxtSEWTZIctOPhtw53EAdRfw4v2e/7n\n"
    "oe9kR3VH1OUKfMDhduMUI09UGGyxsyRcKs1uLvvC5DX2XRAGafN1aDkCgYEA2jWD\n"
    "5SAPPJU+cbkQbkSBmcJph8x0949c/HJ2U6xcMmwR8G4Jnlwe+w3expwKnNlpNXaZ\n"
    "I+mm2BeXvyPJCgN/BMkDhU1xyDQBscCYrD9q41IU318CTmH6iExoUwv6NNuyOsFd\n"
    "IJeYnG6ckgI7yGkY0wxQvsI8alleI2mLehHuwzUCgYEA3Ye1xPlXT7r4MH1PoPmG\n"
    "WEmGlyS7DtKFLuFf1fagT+MeHpgAdfvGf1HNd77ZOgZdQI6k0w9HuMnctnO2U58z\n"
    "K+1P0VJL6sJaP0actt58g2U4C4m73A+lEZbxVCFZNyetXQIsjTMKJ8g5PesyR8+Q\n"
    "c5d/Af4fBldkcZtHIxK9uqkCgYAaqpmQwac7Bx4Xdb9NSm/wI3MUFmdg7ZM2gqJ1\n"
    "PUYTH2Pd1wSz5pweoCZObTlay7LwxqqWWfJ6y/9Oa4ghAiZeplYYz0sNZVWjrF68\n"
    "BhAA8cH9PjYg8BZW28eQBpGwLf0M8x53Yi9TRq05pq45oqZW/FVNypzpfjxj5X0X\n"
    "EOP11QKBgQCDnAVbfrXC+4S5UNwxGHw4cZJwAvOkkeApV3WlBSZFbbGzIxrVy79O\n"
    "7ETTGfSAbksUljV+2HZZVSXtgsCS/fzsFjMWYpeNRX3+9wtFfGCfxoygGW0JvOyY\n"
    "kg61geirHUDYgog9XzGKATXc3K/m7JdyOcWdbf54nhzcEqjRv1DhCA==\n"
    "-----END RSA PRIVATE KEY-----\n";

const char *azure_root_ca = "C0,-----BEGIN CERTIFICATE-----\r\n"
    "MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\r\n"
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\r\n"
    "MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\r\n"
    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n"
    "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\r\n"
    "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\r\n"
    "2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\r\n"
    "1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\r\n"
    "q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\r\n"
    "tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\r\n"
    "vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\r\n"
    "BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\r\n"
    "5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\r\n"
    "1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\r\n"
    "NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\r\n"
    "Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\r\n"
    "8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\r\n"
    "pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\r\n"
    "MrY=\r\n"
    "-----END CERTIFICATE-----\r\n"
    "-----BEGIN CERTIFICATE-----\r\n"
    "MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\r\n"
    "RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\r\n"
    "VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\r\n"
    "DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\r\n"
    "ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\r\n"
    "VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\r\n"
    "mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\r\n"
    "IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\r\n"
    "mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\r\n"
    "XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\r\n"
    "dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\r\n"
    "jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\r\n"
    "BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\r\n"
    "DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\r\n"
    "9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\r\n"
    "jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\r\n"
    "Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\r\n"
    "ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\r\n"
    "R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\r\n"
    "-----END CERTIFICATE-----\r\n";
#endif

extern MODE_FLAG mode_flag;
/* extern int state_flag; */
extern int32_t temperature;
extern uint8_t door_state;
extern uint8_t window_state;
extern uint8_t door_state_changed_bymcu;
extern uint8_t rs_flag;
extern uint32_t fw_update_on;

static uint32_t CurrentBaudRate = UART_BAUDRATE_IF;
static int Send_OK = 0;
static int atq_result = 1;
uint8_t LED_BLINK = 0;
const command_t *at_command_table;
DeviceStatus dev_status = ATCMD_Status_init_stat;
int ret_set = 0;
int ret_cfg = 0;
int ret_cmd = 0;
uint32_t image_received_id;
uint32_t imgcrc, imgid;
char line[UART1_OTA_BUF_SIZE] = {0, };

uint32_t atcmd_wait_bit;
uint32_t atcmd_status_bit = 0;
uint32_t revTotalLen = 0;
uint32_t thr_test_rdy = 0;
uint32_t start_tick, last_tick;
uint8_t isEscRes = 1;

extern void usbx_print(uint8_t *msg, uint8_t len);
extern fsp_err_t SPI_Write_To_DA16xxx(uint8_t *cmd_buf, uint32_t cmd_len);
#if (SUPPORT_MATTER_APP == 1)
extern int matter_event_proc(int argc, char *argv[]);
#endif // SUPPORT_MATTER_APP

int aws_iot_atcmd_stm_it(int argc, char *argv[]);
int get_res_check(void);
uint16_t fast_crc_nbit_lookup(const void *data, int length, uint16_t CrcLUT[4][16], uint16_t previousCrc16);
int atcmd_ota_image_check(uint8_t *imgbuffer, uint32_t crc, uint32_t totcrc, uint32_t size);
atcmd_error_code atcmd_ota_set(char *imgbuffer);
int Send_to_DA16200(uint8_t *sData, uint16_t size, uint8_t retry_skip);

static const command_t commands[] = {
/* cmd, function, arg_count, format, brief */
    {(char *)"+INIT:DONE", NULL, 0, NULL, NULL},
    {(char *)"+"PLATFORM"IOT", aws_iot_atcmd_stm_it, 3, (char*)"<param1>, <param2>,<param3>", " "},
#if (SUPPORT_MATTER_APP == 1)
    {(char *)"+MSTATUS", matter_event_proc, 3, (char*)"<param1>,<param2>,<param3>", " "},
    {(char *)"+MCONTROL", matter_event_proc, 3, (char*)"<param1>,<param2>,<param3>", " "},
    {(char *)"+MATTR", matter_event_proc, 3, (char*)"<param1>,<param2>,<param3>", " "},
    {(char *)"+MCMD", matter_event_proc, 3, (char*)"<param1>,<param2>,<param3>", " "},
#endif // SUPPORT_MATTER_APP
    {NULL, NULL, 0, NULL, NULL}
};

void PRINTF(char *fmt, ...)
{
    va_list ap;
    static char str[MAX_BUF_SIZE];

    memset(str, 0, sizeof(str));
    va_start(ap, fmt);
    vsprintf(str, fmt, ap);
#if (USE_UART_PRINTF == 1)
    printf(str);
#else
    print_to_console(str);
#endif
    va_end(ap);
}

void PRINTF_ATCMD(char *fmt, ...)
{
    va_list ap;
    char str[256];

    memset(str, 0, sizeof(str));
    va_start(ap, fmt);
    vsprintf(str, fmt, ap);

#if defined(AT_ACK_CHECK)
    Send_to_DA16200((uint8_t*)str, strlen(str), RETRY_ACK_CHECK);
#else
    Send_to_DA16200((uint8_t*)str, (uint16_t)strlen(str), RETRY_ACK_CHECK_SKIP);
#endif /* AT_ACK_CHECK */

    va_end(ap);
}

uint32_t PRINTF_ATCMD_WAITRES(uint32_t res_bit, uint32_t timeout_ms)
{
    uint32_t res = 0;
    atcmd_wait_bit = 1;
    while (timeout_ms) {
        if (atcmd_status_bit & res_bit) {
            //PRINTF("Got Status 0x%x  0x%x \r\n", atcmd_status_bit, res_bit);
            res = atcmd_status_bit;
            break;
        }

        timeout_ms--;
        if (timeout_ms == 0) {
            PRINTF("Got Status timeout 0x%x\r\n", res_bit);
            res = 0;
        }
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MICROSECONDS);
    }
    atcmd_wait_bit = 0;
    CLR_BIT(atcmd_status_bit, res_bit);
    return res;
}

void PRINTF_ATCMD_PUT(char *str, uint32_t len)
{
#if (SUPPORT_SPI == 1)
    SPI_Write_To_DA16xxx_direct((uint8_t *)str, len);
#elif (SUPPORT_UART == 1)
    UART_Write_To_DA16xxx((uint8_t *)str, len);
#elif (SUPPORT_SDIO == 1)
    SDIO_Write_To_DA16xxx((uint8_t *)str, len);
#else
    FSP_PARAMETER_NOT_USED(str);
    FSP_PARAMETER_NOT_USED(len);
#endif
}

void wake_up_wifi(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_07_PIN_12, ON);
    vTaskDelay(1);
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_07_PIN_12, OFF);
}

void blink_auto_LED(uint8_t on)
{
    if (on == true && (mode_flag == SET_MODE || mode_flag == AP_MODE || mode_flag == COMMISSIONING_MODE))
        LED_BLINK = true;
    else
        LED_BLINK = false;
}

int get_res_check(void)
{
    uint8_t rcvBuf[UART1_GET_BUF_SIZE] = {0, };
    atcmd_error_code result = ERR_CMD_OK;

    //HAL_UART_Receive(&huart1, rcvBuf, UART1_GET_BUF_SIZE, TIMEOUT_DATA_RECEIVE);
#if 0 /* debug */
    PRINTF("\r\n[%s] %s\r\n", __func__, rcvBuf);
#endif

    if (strlen((char*)rcvBuf) > 0) {
        result = command_parser((char*)rcvBuf);
    }

    if (result != ERR_CMD_OK) {
        return -1;
    } else {
        return 0;
    }
}

int get_send_ok(void)
{
    return Send_OK;
}

void set_uart_baudrate(uint32_t baud)
{
    fsp_err_t err = FSP_SUCCESS;
    baud_setting_t baud_setting;

    if (CurrentBaudRate == baud)
        return;

    CurrentBaudRate = baud;
    PRINTF_ATCMD("\r\nATB=%d\r\n", baud);
    if (fw_update_on == 0)
        PRINTF_ATCMD_WAITRES(BIT_OK | BIT_BAUD, 1000);

    /*
     * enable_bitrate_modulation = false
     * SCI_UART_BAUDRATE_ERROR_PERCENT_5 (5000)
     */
    err = R_SCI_UART_BaudCalculate(baud, false, 5000, &baud_setting);
    if (FSP_SUCCESS != err) {
        PRINTF("\r\n[Err] **  R_SCI_UART_BaudCalculate API failed  **\r\n");
    }

    err = R_SCI_UART_BaudSet(&g_uart7_ctrl, (void*)&baud_setting);
    if (FSP_SUCCESS != err) {
        PRINTF("\r\n [Err]**  R_SCI_UART_BaudSet API failed  **\r\n");
    }
    vTaskDelay(1000);
    PRINTF("\r\nSET UART_CFG %d\r\n", baud);
}

void set_dev_status(int val)
{
    dev_status = val;
#if (SUPPORT_UART == 1)
    if (val == ATCMD_Status_MCUOTA)
        set_uart_baudrate(UART_BAUDRATE_OTA);
    else
        set_uart_baudrate(UART_BAUDRATE_IF);
#endif
}

int get_device_status(void)
{
#if defined(DEBUGGING)
    PRINTF("%s:[%d]\r\n", __func__, dev_status);
#endif
    return dev_status;
}

int aws_iot_atcmd_stm_it(int argc, char *argv[])
{
    char *params[10] = {0, };
    int status = ERR_CMD_OK;
    int k = 0;

    FSP_PARAMETER_NOT_USED(argc);

    params[k] = strtok(argv[1], " ");

    while (params[k] != NULL) {
        k++;
        params[k] = strtok(NULL, " ");
    }
    if (strncmp(params[0], "SERVER_DATA", 11) == 0) { /* new */
        PRINTF("[%s] SERVER_DATA %s %s %s\r\n", __func__, params[1], params[2], params[3]);

        /* Sub *//* Pub */
        /*  0 app_door 1 2 *//* 1 mcu_door 1 0 */
        /*  2 app_window 1 2 *//* 3 mcu_window 1 0 */
        /*  8 app_shadow 1 2 *//* 9 mcu_shadow 1 0 */

        /* Shadow */
        /*  4 battery 0 1 *//* int */
        /*  5 temp 2 1 *//* float */
        /*  6 doorStat 1 1 *//* str */
        /*  7 windowStat 1 1 *//* str */

        if (strncmp(params[1], "0", 1) == 0) { /* index 0 app door */
            if (strncmp(params[2], "app_door", 8) == 0) {
                if (strncmp(params[3], "open", 4) == 0) {
                    PRINTF_ATCMD(REQ_DOOR_OPENED);
                    if (door_state == 0) {
                        TURN_BLUE_ON
                        door_state = 1;
                        door_state_changed_bymcu = 0;

                    }
                } else if (strncmp(params[3], "close", 5) == 0) {
                    PRINTF_ATCMD(REQ_DOOR_CLOSED);
                    if (door_state == 1) {
                        TURN_BLUE_OFF
                        door_state = 0;
                        door_state_changed_bymcu = 0;
                    }
                }
            }
        } else if (strncmp(params[1], "2", 1) == 0) { /* index 2 app_window */
            if (strncmp(params[2], "app_window", 10) == 0) {
                if (strncmp(params[3], "open", 4) == 0) {
                    PRINTF_ATCMD(REQ_WINDOW_OPENED);

                    if (window_state == 0) {
#if defined(USE_DIALOG_SUB_BOARD)
                        HAL_GPIO_WritePin(LED_6_DSEN_GPIO_Port, LED_6_DSEN_Pin, GPIO_PIN_SET);
#endif /* USE_DIALOG_SUB_BOARD */
                        window_state = 1;
                    }
                } else if (strncmp(params[3], "close", 5) == 0) {
                    PRINTF_ATCMD(REQ_WINDOW_CLOSED);

                    if (window_state == 1) {
#if defined(USE_DIALOG_SUB_BOARD)
                        HAL_GPIO_WritePin(LED_6_DSEN_GPIO_Port, LED_6_DSEN_Pin, GPIO_PIN_RESET);
#endif /* USE_DIALOG_SUB_BOARD */
                        window_state = 0;
                    }
                }
            }
        } else if (strncmp(params[1], "8", 1) == 0) { /* index 8 app_shadow */
            if (strncmp(params[2], "app_shadow", 10) == 0) {
                if (strncmp(params[3], "update", 6) == 0) {
                    PRINTF_ATCMD(REQ_SHADOW_UPDATED);
                }
            }
        }
    } else if (strncmp(params[0], "CMD_TO_MCU", 10) == 0) {
        PRINTF("[%s] CMD_TO_MCU [%s]\r\n", __func__, params[1]);

        if (strncmp(params[1], "update", 6) == 0) {
            extern int read_temperature(void);
            read_temperature(); /* for Test */
#if (SUPPORT_UART == 1)
            if (PLATFORM_ID == 1)
                PRINTF_ATCMD("\r\nAT+"PLATFORM" CMD MCU_DATA 2 battery %d 3 temperature %d 4 doorStat %s 7 openMethod %s\r\n",
                    temperature + 35, temperature, door_state == 1 ? RESPONSE_OPEN_TRUE : RESPONSE_CLOSE_FALSE
                    , door_state_changed_bymcu == 1 ? RESPONSE_OPEN_MCU : RESPONSE_OPEN_APP);
            else
                PRINTF_ATCMD("\r\nAT+"PLATFORM" CMD MCU_DATA 4 battery %d 5 temperature %d 6 doorStat %s 7 windowStat %s\r\n",
                    temperature + 35, temperature, door_state == 1 ? RESPONSE_OPEN : RESPONSE_CLOSE
                    , window_state == 1 ? RESPONSE_OPEN : RESPONSE_CLOSE);
#elif (SUPPORT_SPI == 1) || (SUPPORT_SDIO == 1)
            if (PLATFORM_ID == 1)
                PRINTF_ATCMD("AT+"PLATFORM" CMD MCU_DATA 2 battery %d 3 temperature %d 4 doorStat %s 7 openMethod %s",
                    temperature + 35, temperature, door_state == 1 ? RESPONSE_OPEN_TRUE : RESPONSE_CLOSE_FALSE
                    , door_state_changed_bymcu == 1 ? RESPONSE_OPEN_MCU : RESPONSE_OPEN_APP);
            else
                PRINTF_ATCMD("AT+"PLATFORM" CMD MCU_DATA 4 battery %d 5 temperature %d 6 doorStat %s 7 windowStat %s",
                    temperature + 35, temperature, door_state == 1 ? RESPONSE_OPEN : RESPONSE_CLOSE
                    , window_state == 1 ? RESPONSE_OPEN : RESPONSE_CLOSE);
#endif
        }
    } else if (strncmp(params[0], "status", 6) == 0) {
        PRINTF("\r\n[%s] +"PLATFORM"IOT STATUS %d\r\n", __func__, atoi(params[1]));
        set_dev_status(atoi(params[1]));
        if (atcmd_wait_bit)
            SET_BIT(atcmd_status_bit, BIT_STATUS);
    } else if (strncmp(params[0], "OK", 2) == 0) {
        // PRINTF("\r\n[%s] +"PLATFORM"IOT OK\r\n",__func__);
        Send_OK = 1;

    } else {
        //PRINTF("+awsiot not define\r\n");
    }

    return status;
}

#ifdef SUPPORT_CHECK_CRC_16
uint16_t fast_crc_nbit_lookup(const void *data, int length, uint16_t CrcLUT[4][16], uint16_t previousCrc16)
{
    uint16_t crc = (uint16_t)swap16(previousCrc16);
    const unsigned short *current = (const unsigned short*)data;

    while (length > 1) {
        unsigned short one = *current++ ^ (crc);

        crc = CrcLUT[0][(one >> 0) & 0x0f] ^ CrcLUT[1][(one >> 4) & 0x0f] ^ CrcLUT[2][(one >> 8) & 0x0f]
            ^ CrcLUT[3][(one >> 12) & 0x0f];
        length -= 2;
    }

    if (length > 0) {
        unsigned short one = *current;

        one = ((one ^ crc) << 8);
        crc = crc >> 8;

        crc = crc ^ CrcLUT[0][(one >> 0) & 0x0f] ^ CrcLUT[1][(one >> 4) & 0x0f] ^ CrcLUT[2][(one >> 8) & 0x0f]
            ^ CrcLUT[3][(one >> 12) & 0x0f];
    }

    return swap16(crc);
}

static uint16_t swcrc16(uint8_t *data, int32_t length, uint16_t prevCrc16)
{
    uint16_t crcdata;
    crcdata = fast_crc_nbit_lookup(data, length, (uint16_t (*)[16])RO_fast_crc_nbit_LUT, prevCrc16);
    return crcdata;
}
#endif /* SUPPORT_CHECK_CRC_16 */

int atcmd_ota_image_check(uint8_t *imgbuffer, uint32_t crc, uint32_t totcrc, uint32_t size)
{
    uint32_t i, j;
    uint16_t lcrc;
    uint8_t data;

#ifdef IMAGE_CRC_BLOCK_CHECK
    lcrc = swcrc16(imgbuffer, (int32_t)size, 0);
    if (crc != lcrc) {
        PRINTF("\r\nF. IMG_CRC[(0x%x,0x%x)]\r\n", crc, lcrc);
        return -1;
    }
#endif
#ifdef IMAGE_CRC_IMG_CHECK
    imgcrc = swcrc16(imgbuffer, (int32_t)size, (uint16_t)imgcrc);

    if (imgcrc != totcrc) {
        PRINTF("\r\nF. IMG_TCRC[(0x%x,0x%x)]\r\n", totcrc, imgcrc);
        return -1;
    }
#endif
#ifdef IMAGE_PATTERN_CHECK
    for (i = 0; i < size; i += 4096) {
        imgid = ((uint32_t)imgbuffer[i] << 24) + ((uint32_t)imgbuffer[i + 1] << 16) + ((uint32_t)imgbuffer[i + 2] << 8) + (uint32_t)imgbuffer[i + 3];
        if (image_received_id != imgid) {
            PRINTF("F. IMG_PAT_ID[0x%x/0x%x] (0x%x/0x%x/0x%x/0x%x)\r\n", image_received_id, imgid, imgbuffer[0], imgbuffer[1],
                imgbuffer[2], imgbuffer[3]);
            return -1;
        }
        for (j = 4; j < 4096; j++) {
            data = (uint8_t)(imgid + j - 4);
            if (imgbuffer[j] != data) {
                PRINTF("F. IMG_PAT_DAT[%d, %d/(0x%x/0x%x)]\r\n", image_received_id, j, imgbuffer[j], data);
                return -1;
            }
        }
        image_received_id++;
    }
#endif
    return ERR_CMD_OK;
}

atcmd_error_code atcmd_ota_set(char *imgbuffer)
{
    uint8_t *ota_payload;
    ota_mcu_fw_stream_info_t ota_hdr;
    int res;

    memcpy(&ota_hdr, &imgbuffer[strlen(STR_OTA) + 1], sizeof(ota_mcu_fw_stream_info_t));
    ota_payload = (uint8_t*)&imgbuffer[strlen(STR_OTA) + 1 + sizeof(ota_mcu_fw_stream_info_t)];
    if (ota_hdr.offset == 0) {
        image_received_id = 0;
        imgcrc = 0;
        PRINTF("\r\n");
        ath_timer_reset();
    }

    res = atcmd_ota_image_check(ota_payload, ota_hdr.crc, ota_hdr.imgcrc, ota_hdr.size);

    PRINTF("OTA %3d%% [%8d/%8d/%8d/%4d/0x%4x/0x%4x] res(%d)\r", ota_hdr.received_length * 100 / ota_hdr.content_length,
        ota_hdr.offset, ota_hdr.content_length, ota_hdr.received_length, ota_hdr.size, ota_hdr.crc, ota_hdr.imgcrc, res);

    if (ota_hdr.received_length == ota_hdr.content_length) {
        uint32_t spend = xTaskGetTickCount() - start_tick;
        uint32_t bytepms = ota_hdr.received_length / spend;
        uint32_t byteps = bytepms * 1000;
        uint32_t kbytepsh = byteps / 1024;
        PRINTF("\r\n>>>>> (OTA) Receiving Len %d.%dKB\r\n", ota_hdr.received_length / 1024, (ota_hdr.received_length % 1024) * 1000 / 1024);
        PRINTF(">>>>> (OTA) Receiving Time %d.%03d seconds\r\n", spend / 1000, spend % 1000);
        PRINTF(">>>>> (OTA) Receiving THR %d.%dKB/S (%d.%03d Mbps)\r\n"
            , kbytepsh, byteps % 1024
            , (kbytepsh * 8) / 1024 , (kbytepsh * 8) % 1024);
    }
    if (res != ERR_CMD_OK)
        PRINTF_ATCMD(RES_OTA_FAIL);
    else
        PRINTF_ATCMD(RES_OTA_OK);

    return res;
}

atcmd_error_code command_parser(char *_payload)
{
    const command_t *cmd_ptr = NULL;
    char *params[MAX_PARAMS];
    uint32_t param_cnt = 0;
    uint32_t length_all = 0;
    atcmd_error_code result_code = ERR_CMD_OK;
    uint16_t lenPayload = (uint16_t)strlen(_payload);
    uint32_t i = 0, ota_flag = 0;

    while (i < lenPayload) {
        if ((_payload[i] != '\r') && (_payload[i] != '\n'))
            break;
        i++;
    }

    if (_payload[i] == STR_PLUS) {
        if (strncmp(STR_OTA, &_payload[i], strlen(STR_OTA)) == 0) {
            ota_flag = 1;
        } else if (strncmp(STR_TCPSD, &_payload[i], strlen(STR_TCPSD)) == 0) {
            //PRINTF("D");
            result_code = ERR_CMD_OK;
            goto parser_end;
        } else if (strncmp(STR_TCPCD, &_payload[i], strlen(STR_TCPCD)) == 0) {
            //PRINTF("d");
            result_code = ERR_CMD_OK;
            goto parser_end;
        } else if (strncmp(STR_BAUD, &_payload[i], strlen(STR_BAUD)) == 0) {
            char baud[10];
            if (atcmd_wait_bit)
                SET_BIT(atcmd_status_bit, BIT_BAUD);
            result_code = ERR_CMD_OK;
            memcpy(baud, &_payload[i] + strlen(STR_BAUD) + 1, 6);
            baud[6] = 0;
            //CurrentBaudRate = atoi(baud);
            PRINTF("+BAUD %s %d\r\n", &_payload[i], CurrentBaudRate);
            goto parser_end;
        }
    } else {
        if (strncmp(STR_OK, &_payload[i], strlen(STR_OK)) == 0) {
            if (atcmd_wait_bit)
                SET_BIT(atcmd_status_bit, BIT_OK);
            //PRINTF("OK\r\n");
        } else if (strncmp(STR_ERR, &_payload[i], strlen(STR_ERR)) == 0) {
            if (atcmd_wait_bit)
                SET_BIT(atcmd_status_bit, BIT_ERR);
            //PRINTF("Err\r\n");
        } else if (strncmp(STR_OTA, &_payload[i], strlen(STR_OTA)) == 0) {
            ota_flag = 1;
        }
    }

    if (ota_flag) {
        result_code = atcmd_ota_set(&_payload[i]);
        goto parser_end;
    }

    if (i < lenPayload) {
        strncpy(line, &_payload[i], lenPayload - i + 1);
    } else {
        strncpy(line, _payload, lenPayload + 1);
    }
    length_all = strlen(line);

    at_command_table = commands;

    /* First call to strtok. */
    if (strchr(line, '=') == NULL) {
        params[param_cnt++] = strtok(line, DELIMIT); /* ' ' */
    } else {
        params[param_cnt++] = strtok(line, DELIMIT_EQ); /* '=' */
    }

    if (params[0] == NULL) {
        result_code = ERR_UNKNOWN_CMD;
    } else {
        /* find the command */

        for (cmd_ptr = at_command_table; cmd_ptr->name != NULL; cmd_ptr++) {
            if (strcasecmp(params[0], cmd_ptr->name) == 0) {
                break;
            }
        }

        if (cmd_ptr->name == NULL) {
            line[strlen(params[0])] = '=';
            result_code = ERR_UNKNOWN_CMD;
        } else {
            /* parse arguments */
            while (((params[param_cnt] = strtok(NULL, DELIMIT_COMMA)) != NULL)) {
                if (params[param_cnt][0] == '\'') {
                    char *tmp_ptr;

                    /* Restore delimit-character */
                    params[param_cnt][strlen(params[param_cnt])] = ',';

                    if (strncmp(params[param_cnt], "',", 2) == 0) {
                        /* First argument : AT+XXX=',aaaaa','bbbb' */
                        if (param_cnt == 1) {
                            if ((tmp_ptr = strstr(&params[param_cnt][1], "',")) != NULL) {
                                params[param_cnt] = params[param_cnt] + 1;
                                strtok(tmp_ptr, DELIMIT_COMMA);
                                *tmp_ptr = '\0';
                                *(tmp_ptr + 1) = '\0';
                            }
                        } else {
                            if (params[param_cnt][strlen(params[param_cnt]) - 1] == '\'') {
                                tmp_ptr = params[param_cnt] + strlen(params[param_cnt]) - 1;
                                params[param_cnt] = params[param_cnt] + 1;
                                strtok(tmp_ptr, "'");
                                *tmp_ptr = '\0';
                            } else {
                                result_code = ERR_WRONG_ARGUMENTS;
                                goto atcmd_result;
                            }
                        }
                    } else if ((tmp_ptr = strstr(params[param_cnt], "',")) != NULL) {
                        params[param_cnt] = params[param_cnt] + 1;
                        strtok(tmp_ptr, DELIMIT_COMMA);
                        *tmp_ptr = '\0';
                        *(tmp_ptr + 1) = '\0';
                    } else if (params[param_cnt][strlen(params[param_cnt]) - 1] == '\'') {
                        tmp_ptr = params[param_cnt] + strlen(params[param_cnt]) - 1;
                        params[param_cnt] = params[param_cnt] + 1;
                        strtok(tmp_ptr, "'");
                        *tmp_ptr = '\0';
                    } else {
                        result_code = ERR_WRONG_ARGUMENTS;
                        goto atcmd_result;
                    }
                } else {
                    if (strstr(params[param_cnt], "'") != NULL) {
                        result_code = ERR_WRONG_ARGUMENTS;
                        goto atcmd_result;
                    }
                }

                param_cnt++;

                if (param_cnt > (MAX_PARAMS - 1)) {
                    result_code = ERR_TOO_MANY_ARGS;
                    break;
                }
            }

            /* check arguments length */
            if (param_cnt > 1 && length_all - strlen(params[0]) - 1 > TX_PAYLOAD_MAX_SIZE) {
                result_code = ERR_WRONG_ARGUMENTS;
                goto atcmd_result;
            }

            /* check arguments */
            if ((param_cnt - 1 > (uint32_t)cmd_ptr->arg_count) && (params[1][0] != '?') && (strcasecmp(params[1], "HELP"))) {
                result_code = ERR_TOO_MANY_ARGS;
            }

            /* run command */
            if ((result_code == ERR_CMD_OK) && (cmd_ptr->command != NULL)) {
                result_code = (atcmd_error_code)cmd_ptr->command((int)param_cnt, params);
            }
        }

atcmd_result: if (atq_result == 1) {
            if (result_code == ERR_CMD_OK) {
                /*PRINTF("\r\nAT+"PLATFORM" OK\r\n");*/
            } else if (result_code == ERR_CMD_OK_WO_PRINT) {
                /* Not Printed */
            } else if (result_code != ERR_UNKNOWN_CMD) {
                /*PRINTF("\r\nAT+"PLATFORM" ERROR\r\n");*/
            }
        }
    }
parser_end:
    rs_flag = 0;
    return result_code;
}

atcmd_error_code atcmd_config_set(void)
{
    atcmd_error_code ret = ERR_CMD_OK;
    uint8_t *cmd;
    uint8_t cnt = 0;

    cmd = pvPortMalloc(256);
    blink_auto_LED(true);
    for (cnt = 0; cnt < MAX_CFG_NUM; cnt++) {
        if (PLATFORM_ID == 0) {
            if (cmd_set_aws_cfg[cnt] == NULL) {
                break;
            }
            PRINTF_ATCMD((char *)cmd_set_aws_cfg[cnt]);
        } else {
            if (cmd_set_azure_cfg[cnt] == NULL) {
                break;
            }
            PRINTF_ATCMD((char *)cmd_set_azure_cfg[cnt]);
        }
        vTaskDelay(400);
        //blink_auto_LED();
    }
    vPortFree(cmd);
    return ret;
}

atcmd_error_code atcmd_certificate(uint8_t idx)
{
    uint8_t ret = ERR_CMD_OK;
    /* uint8_t cnt = 0; */
#if (SUPPORT_SPI == 1) || (SUPPORT_SDIO == 1)
    cert_ca1[0] = cert_cert1[0] = cert_key1[0] = azure_root_ca[0] = 0x1b;
    //cert_ca1[strlen(cert_ca1)-1] = 0x03;
    //cert_cert1[strlen(cert_cert1)-1] = 0x03;
    //cert_key1[strlen(cert_key1)-1] = 0x03;
    //azure_root_ca[strlen(azure_root_ca)-1] = 0x03;
#else
    //UART_Write_To_DA16xxx ((uint8_t*) "\x1b", strlen ("\x1b"));
    Send_to_DA16200((uint8_t*)"\x1b", (uint16_t)strlen("\x1b"), RETRY_ACK_CHECK_SKIP);
#endif
    if (PLATFORM_ID == 0) {
        if (idx == 0)
            //UART_Write_To_DA16xxx ((uint8_t*) cert_ca1, strlen (cert_ca1));
            Send_to_DA16200((uint8_t*)cert_ca1, (uint16_t)strlen(cert_ca1), RETRY_ACK_CHECK_SKIP);
        else if (idx == 1)
            //UART_Write_To_DA16xxx ((uint8_t*) cert_cert1, strlen (cert_cert1));
            Send_to_DA16200((uint8_t*)cert_cert1, (uint16_t)strlen(cert_cert1), RETRY_ACK_CHECK_SKIP);
        else if (idx == 2)
            //UART_Write_To_DA16xxx ((uint8_t*) cert_key1, strlen (cert_key1));
            Send_to_DA16200((uint8_t*)cert_key1, (uint16_t)strlen(cert_key1), RETRY_ACK_CHECK_SKIP);
    } else {
        if (idx == 0)
            //UART_Write_To_DA16xxx ((uint8_t*) azure_root_ca, strlen (azure_root_ca));
            Send_to_DA16200((uint8_t*)azure_root_ca, (uint16_t)strlen(azure_root_ca), RETRY_ACK_CHECK_SKIP);
    }
#if (SUPPORT_UART == 1)
    Send_to_DA16200((uint8_t*)"\x03", (uint16_t)strlen("\x03"), RETRY_ACK_CHECK_SKIP);
#endif

    blink_auto_LED(false);

    return ret;
}

int Send_to_DA16200(uint8_t *sData, uint16_t size, uint8_t retry_skip)
{
    int cnt, retryCnt;

    Send_OK = 0;
    retryCnt = 0;

ReTry:

#if (SUPPORT_UART == 1)
    UART_Write_To_DA16xxx((uint8_t*)sData, (uint32_t)size);
    /* PRINTF("%s", (uint8_t *)sData); *//* for debugging */
#elif (SUPPORT_SPI == 1)
    SPI_Write_To_DA16xxx((uint8_t *)sData, (uint32_t)size);
#elif (SUPPORT_SDIO == 1)
    SDIO_Write_To_DA16xxx((uint8_t *)sData, (uint32_t)size);
#else
    FSP_PARAMETER_NOT_USED(size);
#endif

    if (retry_skip) {
        /* HAL_Delay(50); *//*wait 50msec */
        Send_OK = 1;
    } else {
        for (cnt = 0; cnt < 20; cnt++) {
            get_res_check();

            if (Send_OK == 1) {
                cnt = 0;
                PRINTF("\r\nSend OK:\"%s\"\r\n", sData);
                break;
            }

            /* HAL_Delay(5); *//* wait 5msec */
        }
    }

    if (Send_OK == 0 && retryCnt < MAX_RETRY_SEND_COUNT) {
        PRINTF("\r\nSend Re-Try\r\n");
        retryCnt++;
        goto ReTry;
    }

    return Send_OK;
}

/// Hexa Dump API //////////////////////////////////////////////////////////
/*
 direction: 0: UART0, 1: ATCMD_INTERFACE
 output_fmt: 0: ascii only, 1 hexa only, 2 hexa with ascii
 */

void hexa_dump_print(uint8_t *title, const void *buf, size_t len, char direction, char output_fmt)
{
    size_t i, llen;
    const uint8_t *pos = buf;
    const size_t line_len = 16;
    int hex_index = 0;
    char *buf_prt = NULL;

    buf_prt = pvPortMalloc(64);
    if (buf_prt == NULL) {
        PRINTF("[%s] Failed to allocate the temporary buffer ...\n", __func__);
        return;
    }

    if (output_fmt) {
        PRINTF(">>> %s \n", title);
    }

    if (buf == NULL) {
        PRINTF(" - hexdump%s(len=%lu): [NULL]\n", output_fmt == OUTPUT_HEXA_ONLY ? "" : "_ascii", (unsigned long)len);

        vPortFree(buf_prt);

        return;
    }

    if (output_fmt) {
        PRINTF("- (len=%lu):\n", (unsigned long)len);
    }

    while (len) {
        char tmp_str[4];

        llen = len > line_len ? line_len : len;

        memset(buf_prt, 0, 64);

        if (output_fmt) {
            sprintf(buf_prt, "[%08x] ", hex_index);

            for (i = 0; i < llen; i++) {
                sprintf(tmp_str, " %02x", pos[i]);
                strcat(buf_prt, tmp_str);
            }

            hex_index = (int)hex_index + (int)i;

            for (i = llen; i < line_len; i++) {
                strcat(buf_prt, "   "); /* _xx */
            }

            if (direction) {
#ifdef  __TEST_USER_AT_CMD__
                PRINTF_ATCMD("%s  ", buf_prt);
#endif
            } else {
                PRINTF("%s  ", buf_prt);
            }

            memset(buf_prt, 0, 64);
        }

        if (output_fmt == OUTPUT_HEXA_ASCII || output_fmt == OUTPUT_ASCII_ONLY) {
            for (i = 0; i < llen; i++) {
                if ((pos[i] >= 0x20 && pos[i] < 0x7f)
                    || (output_fmt == OUTPUT_ASCII_ONLY && (pos[i] == 0x0d || pos[i] == 0x0a || pos[i] == 0x0c))) {

                    sprintf(tmp_str, "%c", pos[i]);
                    strcat(buf_prt, tmp_str);
                } else if (output_fmt) {
                    strcat(buf_prt, ".");
                }
            }
        }

        if (output_fmt) {
            for (i = llen; i < line_len; i++) {
                strcat(buf_prt, " ");
            }

            if (direction) {
                strcat(buf_prt, "\n\r"); /* ATCMD */
            } else {
                strcat(buf_prt, "\n"); /* Normal */
            }
        }

        if (direction) {
#ifdef  __TEST_USER_AT_CMD__
            PRINTF_ATCMD(buf_prt);
#endif
        } else {
            PRINTF(buf_prt);
        }

        pos += llen;
        len -= llen;
    }

    vPortFree(buf_prt);
}

void ath_timer_reset(void)
{

    start_tick = xTaskGetTickCount();
}

