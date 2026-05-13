/***********************************************************************************************************************

* File Name    : mqtt_fsp_conf.h

* Description  : Contains data structures and functions used in mqtt_client.c and app_task_entry.c

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates

*

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/

#ifndef MQTT_CERTS_H
#define MQTT_CERTS_H

/* ================= Root CA ================= */

#define ROOT_CA \
"-----BEGIN CERTIFICATE-----\n" \
"MIICnDCCAkGgAwIBAgIJAM0wYdY5quqFMAoGCCqGSM49BAMCMGkxCzAJBgNVBAYT\n" \
"AktSMQ4wDAYDVQQIDAVTZW91bDEOMAwGA1UEBwwFU2VvdWwxEDAOBgNVBAoMB1Jl\n" \
"bmVzYXMxDTALBgNVBAsMBElJQlUxGTAXBgNVBAMMEFJvb3QgQ2VydGlmaWNhdGUw\n" \
"HhcNMjMwMzIzMDQwMTQwWhcNMzMwMzIwMDQwMTQwWjBpMQswCQYDVQQGEwJLUjEO\n" \
"MAwGA1UECAwFU2VvdWwxDjAMBgNVBAcMBVNlb3VsMRAwDgYDVQQKDAdSZW5lc2Fz\n" \
"MQ0wCwYDVQQLDARJSUJVMRkwFwYDVQQDDBBSb290IENlcnRpZmljYXRlMFkwEwYH\n" \
"KoZIzj0CAQYIKoZIzj0DAQcDQgAEqihf28i8BY434sE9V+fhS6S94hZpUKJAcuBr\n" \
"ZTZcKyE/PjvJmqSyWf5FvkeY49doR+9rCgaK5fDmn6aD+TgFu6OB0TCBzjAPBgNV\n" \
"HRMBAf8EBTADAQH/MB0GA1UdDgQWBBTuwab2GUr6mZBGAp41AqSZtp/hhjCBmwYD\n" \
"VR0jBIGTMIGQgBTuwab2GUr6mZBGAp41AqSZtp/hhqFtpGswaTELMAkGA1UEBhMC\n" \
"S1IxDjAMBgNVBAgMBVNlb3VsMQ4wDAYDVQQHDAVTZW91bDEQMA4GA1UECgwHUmVu\n" \
"ZXNhczENMAsGA1UECwwESUlCVTEZMBcGA1UEAwwQUm9vdCBDZXJ0aWZpY2F0ZYIJ\n" \
"AM0wYdY5quqFMAoGCCqGSM49BAMCA0kAMEYCIQDEgggVYtQtcM1RlHm2bvduDF+Y\n" \
"yAZAChjgyY3Ilm0OlwIhAPSJYYtLHyPeDMwenXMDk5dcll1ZLQZqfcubGHq2YA9n\n" \
"-----END CERTIFICATE-----\n"


/* ================= Client Certificate ================= */

#define CLIENT_CERT \
"-----BEGIN CERTIFICATE-----\n" \
"MIICoTCCAkigAwIBAgIJALrAHEerhG8bMAoGCCqGSM49BAMCMGkxCzAJBgNVBAYT\n" \
"AktSMQ4wDAYDVQQIDAVTZW91bDEOMAwGA1UEBwwFU2VvdWwxEDAOBgNVBAoMB1Jl\n" \
"bmVzYXMxDTALBgNVBAsMBElJQlUxGTAXBgNVBAMMEFJvb3QgQ2VydGlmaWNhdGUw\n" \
"HhcNMjMwMzIzMDQwMTQ5WhcNMzMwMzIwMDQwMTQ5WjBzMQswCQYDVQQGEwJLUjES\n" \
"MBAGA1UECAwJU29tZXdoZXJlMRIwEAYDVQQHDAlTb21ld2hlcmUxEDAOBgNVBAoM\n" \
"B1JlbmVzYXMxDTALBgNVBAsMBElJQlUxGzAZBgNVBAMMEkNsaWVudCBDZXJ0aWZp\n" \
"Y2F0ZTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABElgzxeL2s4JemIC0n32Yrng\n" \
"cmCq/XwRCMGri1ZNDbnhRyNaHfclRwhEGWKiP64bTuq3HXb5yB0dQ5mp+5vHbh2j\n" \
"gc4wgcswDAYDVR0TAQH/BAIwADAdBgNVHQ4EFgQUHrDENetRTmSpJKQ3T9mHew4f\n" \
"geUwgZsGA1UdIwSBkzCBkIAU7sGm9hlK+pmQRgKeNQKkmbaf4YahbaRrMGkxCzAJ\n" \
"BgNVBAYTAktSMQ4wDAYDVQQIDAVTZW91bDEOMAwGA1UEBwwFU2VvdWwxEDAOBgNV\n" \
"BAoMB1JlbmVzYXMxDTALBgNVBAsMBElJQlUxGTAXBgNVBAMMEFJvb3QgQ2VydGlm\n" \
"aWNhdGWCCQDNMGHWOarqhTAKBggqhkjOPQQDAgNHADBEAiB+zugK0WO7rmMCDxjO\n" \
"ig4kkvocIw1I21oV9btCs79j6AIgfdh9ssnstpVncHwHvteGCscId77lWvShjCsU\n" \
"DhCTJGA=\n" \
"-----END CERTIFICATE-----\n"


/* ================= Private Key ================= */

#define PRIVATE_KEY \
"-----BEGIN EC PRIVATE KEY-----\n" \
"MHcCAQEEIJ5J12YmRrEzGKrkX08/QEPpJTgE74VIqs9rh6mtPf4poAoGCCqGSM49\n" \
"AwEHoUQDQgAESWDPF4vazgl6YgLSffZiueByYKr9fBEIwauLVk0NueFHI1od9yVH\n" \
"CEQZYqI/rhtO6rcddvnIHR1Dman7m8duHQ==\n" \
"-----END EC PRIVATE KEY-----\n"

#endif /* MQTT_CERTS_H */
