/***********************************************************************************************************************
 * File Name    : config.h
 * Description  : Contains HTTP(s) server configuration and security credentials
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "rm_cert.h"
#include "rm_http_client.h"
#include "rm_https_api.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define HTTPS
#ifdef HTTPS
#define SERVER_PORT 443
#else
#define SERVER_PORT 80
#endif //HTTPS

#define CERT_MAX_LENGTH (1024 * 4)
#define FLASH_WRITE_LENGTH (1024 * 4)

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
#ifdef HTTPS
static httpc_secure_connection_t sec_conn;
static https_server_sec_t sever_sec;

static uint8_t tls_srv_cert[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDtzCCAp+gAwIBAgIUeblhDZyEJUjr+MJ6sADm9nzDuIYwDQYJKoZIhvcNAQEL\n"
    "BQAwZTELMAkGA1UEBhMCanAxDjAMBgNVBAgMBU9zYWthMRIwEAYDVQQHDAlPc2Fr\n"
    "YS1zaGkxFjAUBgNVBAoMDSJFeGFtcGxlIEluYyIxDDAKBgNVBAsMA0ZvbzEMMAoG\n"
    "A1UEAwwDdGVrMB4XDTI0MTAwMjEzMDI0OVoXDTI1MTAwMjEzMDI0OVowgZMxCzAJ\n"
    "BgNVBAYTAkpQMQ4wDAYDVQQIDAVUb2t5bzEQMA4GA1UEBwwHS29kYWlyYTEQMA4G\n"
    "A1UECgwHUmVuZXNhczENMAsGA1UECwwEU1dURDESMBAGA1UEAwwJbG9jYWxob3N0\n"
    "MS0wKwYJKoZIhvcNAQkBFh5ha2loaXRvLm9rdW11cmEudWpAcmVuZXNhcy5jb20w\n"
    "WTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAS8YFWOfrPXnYMfyIG8ReoRRcPGQndj\n"
    "bwzlH85UL5a1BMprjtm6v+V1He7+c5a4EjBFpCTVP2I/Wqlrx80B/4Kko4H6MIH3\n"
    "MEcGA1UdEQRAMD6CCWxvY2FsaG9zdIIVbG9jYWxob3N0LmxvY2FsZG9tYWluhwR/\n"
    "AAABggNhcHCCD2FwcC5sb2NhbGRvbWFpbjAdBgNVHQ4EFgQUIdO+KVtvxHq23zq9\n"
    "c6Y9urtQiGowgYwGA1UdIwSBhDCBgaFppGcwZTELMAkGA1UEBhMCanAxDjAMBgNV\n"
    "BAgMBU9zYWthMRIwEAYDVQQHDAlPc2FrYS1zaGkxFjAUBgNVBAoMDSJFeGFtcGxl\n"
    "IEluYyIxDDAKBgNVBAsMA0ZvbzEMMAoGA1UEAwwDdGVrghROcyfj2vaChNf/sD5A\n"
    "Endq5udyhzANBgkqhkiG9w0BAQsFAAOCAQEAWRaMsAb3F5k0XK1/C64ixNnVNKhC\n"
    "nsFHOqANkTWELBER9xHwsW8JzjiSIBNXzZIIXgRCB1B4Tg+QpvklGBPi85JieD1r\n"
    "Sv6LTMtyMPLOXutJYwPxOYcxH5McWCsin9fHXswFvyCTDReiyQJRJRjc7TLt+rl6\n"
    "O/pp2k2fQ4YHEjOMiYxJ+FhS1ZorsK9l0PMhVHRyakEEmDb28cq4tEvuPE/dGtcG\n"
    "PNTE/0p+I59gGKLPdyzBeMInskuS7dGBy9uaoh6ITliT/v5oDH2VlRipCm93CzRu\n"
    "+v38SXOqHgFEg6GKH6Qz3NYfvfm95VULXXK0JnykSQR8GQgdHvIh+AXrVw==\n"
    "-----END CERTIFICATE-----\n";

static size_t tls_srv_cert_len = sizeof(tls_srv_cert);

static uint8_t tls_srv_key[] =
    "-----BEGIN EC PARAMETERS-----\n"
    "BggqhkjOPQMBBw==\n"
    "-----END EC PARAMETERS-----\n"
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEIA0ZIG9cCFP78B7flpAmm+xD/EFTO4FSSne3bNMgTnm6oAoGCCqGSM49\n"
    "AwEHoUQDQgAEvGBVjn6z152DH8iBvEXqEUXDxkJ3Y28M5R/OVC+WtQTKa47Zur/l\n"
    "dR3u/nOWuBIwRaQk1T9iP1qpa8fNAf+CpA==\n"
    "-----END EC PRIVATE KEY-----\n";

static size_t tls_srv_key_len = sizeof(tls_srv_key);

static uint8_t ca_cert[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDUTCCAjkCFE5zJ+Pa9oKE1/+wPkASd2rm53KHMA0GCSqGSIb3DQEBCwUAMGUx\n"
    "CzAJBgNVBAYTAmpwMQ4wDAYDVQQIDAVPc2FrYTESMBAGA1UEBwwJT3Nha2Etc2hp\n"
    "MRYwFAYDVQQKDA0iRXhhbXBsZSBJbmMiMQwwCgYDVQQLDANGb28xDDAKBgNVBAMM\n"
    "A3RlazAeFw0yNDA5MTAyMjI0NTBaFw0yNTA5MTAyMjI0NTBaMGUxCzAJBgNVBAYT\n"
    "AmpwMQ4wDAYDVQQIDAVPc2FrYTESMBAGA1UEBwwJT3Nha2Etc2hpMRYwFAYDVQQK\n"
    "DA0iRXhhbXBsZSBJbmMiMQwwCgYDVQQLDANGb28xDDAKBgNVBAMMA3RlazCCASIw\n"
    "DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANJbt7+igiNpXlquBLnbn2urT9sz\n"
    "g5NKG7vL6JckAYm9Am/M/KGrcN3U7z6AKQI0Zt0uigN4b5QF3aeVqbwKXJO8lYCS\n"
    "LeRpyl64pXwIuSQa0x21SNFqojDLl7Bk520DqD76mG1MLq/HZirR6R5+VIJV182x\n"
    "c5ZqhskWLPQJ+ASdkYxYbma2FWeLClfJIdo6L6q1om6OB9jIc3wWqfE9ZFo7JHH2\n"
    "5fJAErRAF4jgVGFZOsXW8eVxjT5uTXHT7NKIqGmMjRjUlO/n5IBcsD+PE/jJh/Br\n"
    "gy+wU3cgfYudmirzueuoQipgrmCTBg5TCiXYrXQchwjenRjm3+PG1JLU/UcCAwEA\n"
    "ATANBgkqhkiG9w0BAQsFAAOCAQEAHTeQqLy6aB+GqDEfs6tE3p3kZOkjSw29hZDg\n"
    "CBnbGsh4BrGO/GBesTuRIV6Gl2g8tsGVeHMSnYw50hMtoLeDIjWL2jOElDlV/xxG\n"
    "6DOYGNYQ3W3uqsu4oEBZyoPTXEikeSB9i5AQUlqvH6vRxj35TD/U61Yd6sibT1OY\n"
    "jzk0NC1VmhHao3XcAsvJxxrkwj+vjMfairl6AYrSUqm0YImnxSnymF1f72rR+ZbP\n"
    "TEh6ddww5+UjlYt3arWx7EtZ1GGBTL1WJQXcdVKYn6/AUbqdhXWghO47KDjy8cAM\n"
    "k7+DzGzGV4OTzGjLKCZYKveYCIPCTaacnv/yALJG+p9qvi26UQ==\n"
    "-----END CERTIFICATE-----\n";

static size_t ca_cert_len = sizeof(ca_cert);

#endif //HTTPs
