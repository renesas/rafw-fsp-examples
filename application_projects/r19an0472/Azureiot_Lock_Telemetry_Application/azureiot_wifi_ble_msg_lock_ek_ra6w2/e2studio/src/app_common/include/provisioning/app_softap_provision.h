/**
 ****************************************************************************************
 *
 * @file app_softap_provision.h
 *
 * @brief User APIs for DA16200 provision.
 *
 * Copyright (c) 2026 Renesas Electronics. All rights reserved.
 * 
 * This software ("Software") is owned by Renesas Electronics.
 * 
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 * 
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 * 
 ****************************************************************************************
 */

#if !defined (__APP_SOFTAP_PROVISION_H__)
#define	__APP_SPFTAP_PROVISION_H__

//#include "sdk_type.h"

#if defined (__SUPPORT_WIFI_PROVISIONING__)

#include "common_def.h"
#include "da16x_system.h"
#include "nvedit.h"
#include "environ.h"
#include "iface_defs.h"
#include "command.h"
#include "common_utils.h"
#include "task.h"
#include "lwip/sockets.h"
#include "da16x_sntp_client.h"

#include "mbedtls/config.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/net.h"

/**
 * @brief Definition for the SoftAP Provisioning 
 */
#define	TCP_SERVER_WIN_SIZE		( 1024 * 4 )
#define	TCP_LISTEN_BAGLOG			5
#define	PROV_TLS_SVR_TIMEOUT		1000	//500	   EU FAE TLS Fail issue 500 ==> 1000

/**
 * @brief TLS provisioning certification Root_ca file
 */
unsigned char prov_tls_ca[] = "-----BEGIN CERTIFICATE-----\n"
    "MIID+TCCAuGgAwIBAgIJANqqHCazDkkOMA0GCSqGSIb3DQEBCwUAMIGSMQswCQYD\n"
    "VQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEUMBIGA1UEBwwLU2FudGEgQ2xh\n"
    "cmExFzAVBgNVBAoMDldpLUZpIEFsbGlhbmNlMR0wGwYDVQQDDBRXRkEgUm9vdCBD\n"
    "ZXJ0aWZpY2F0ZTEgMB4GCSqGSIb3DQEJARYRc3VwcG9ydEB3aS1maS5vcmcwHhcN\n"
    "MTMwMzExMTkwMjI2WhcNMjMwMzA5MTkwMjI2WjCBkjELMAkGA1UEBhMCVVMxEzAR\n"
    "BgNVBAgMCkNhbGlmb3JuaWExFDASBgNVBAcMC1NhbnRhIENsYXJhMRcwFQYDVQQK\n"
    "DA5XaS1GaSBBbGxpYW5jZTEdMBsGA1UEAwwUV0ZBIFJvb3QgQ2VydGlmaWNhdGUx\n"
    "IDAeBgkqhkiG9w0BCQEWEXN1cHBvcnRAd2ktZmkub3JnMIIBIjANBgkqhkiG9w0B\n"
    "AQEFAAOCAQ8AMIIBCgKCAQEA6TOCu20m+9zLZITYAhGmtxwyJQ/1xytXSQJYX8LN\n"
    "YUS/N3HG2QAQ4GKDh7DPDI13zhdc0yOUE1CIOXa1ETKbHIU9xABrL7KfX2HCQ1nC\n"
    "PqRPiW9/wgQch8Aw7g/0rXmg1zewPJ36zKnq5/5Q1uyd8YfaXBzhxm1IYlwTKMlC\n"
    "ixDFcAeVqHb74mAcdel1lxdagHvaL56fpUExm7GyMGXYd+Q2vYa/o1UwCMGfMOj6\n"
    "FLHwKpy62KCoK3016HlWUlbpg8YGpLDt2BB4LzxmPfyH2x+Xj75mAcllOxx7GK0r\n"
    "cGPpINRsr4vgoltm4Bh1eIW57h+gXoFfHCJLMG66uhU/2QIDAQABo1AwTjAdBgNV\n"
    "HQ4EFgQUCwPCPlSiKL0+Sd5y8V+Oqw6XZ4IwHwYDVR0jBBgwFoAUCwPCPlSiKL0+\n"
    "Sd5y8V+Oqw6XZ4IwDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAsNxO\n"
    "z9DXb7TkNFKtPOY/7lZig4Ztdu6Lgf6qEUOvJGW/Bw1WxlPMjpPk9oI+JdR8ZZ4B\n"
    "9QhE+LZhg6SJbjK+VJqUcTvnXWdg0e8CgeUw718GNZithIElWYK3Kh1cSo3sJt0P\n"
    "z9CiJfjwtBDwsdAqC9zV9tgp09QkEkav84X20VxaITa3H1QuK/LWSn/ORrzcX0Il\n"
    "10YoF6Hz3ZWa65mUoMzd8DYtCyGtcbYrSt+NMCqRB186PDQn5XBCytgF8VuiCyyk\n"
    "Z04hqHLzAFc21P9yhwKGi3BHD/Sep8fvr9y4VpMIqHQm2jaFPxY1VxhPSV+UHoE1\n"
    "fCPitIJTp/iXi7uXTQ==\n"
    "-----END CERTIFICATE-----\n";

/**
 * @brief TLS provisioning certification cert file 
 */
unsigned char prov_tls_cert[] = "-----BEGIN CERTIFICATE-----\n"
    "MIIFGzCCBAOgAwIBAgICECEwDQYJKoZIhvcNAQELBQAwgZIxCzAJBgNVBAYTAlVT\n"
    "MRMwEQYDVQQIDApDYWxpZm9ybmlhMRQwEgYDVQQHDAtTYW50YSBDbGFyYTEXMBUG\n"
    "A1UECgwOV2ktRmkgQWxsaWFuY2UxHTAbBgNVBAMMFFdGQSBSb290IENlcnRpZmlj\n"
    "YXRlMSAwHgYJKoZIhvcNAQkBFhFzdXBwb3J0QHdpLWZpLm9yZzAeFw0xMzA0MTUw\n"
    "MTQ4MDRaFw0yMzA0MTMwMTQ4MDRaMIGBMQswCQYDVQQGEwJVUzETMBEGA1UECAwK\n"
    "Q2FsaWZvcm5pYTEXMBUGA1UECgwOV2ktRmkgQWxsaWFuY2UxIzAhBgNVBAMMGkFB\n"
    "QSBTZXJ2ZXIgQ2VydGlmaWNhdGUgSURCMR8wHQYJKoZIhvcNAQkBFhBzdXBwb3J0\n"
    "QHdpLWZpLm9yMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAyCFCRmy1\n"
    "hNJpZVFK5VAUILS0ukrHXAlSj6EQ9imSulkp1s1vtNw0iP0Zb6o1G+duMeowUMe1\n"
    "Ww941XBXQamRo5HnzuQ0Je2gcJdV/CxFSZPOGc8WVnQIXC/snYLHZf4yUpgxTmh1\n"
    "2wJIRgy8o8d0YjN4OLQeRZVSTW4zzjRCQqrRQjnwbRBhLxwMWVK+W6rG7Jtf7rE1\n"
    "u9grGbicUysPtPQn/B/Sm9EX0y45hTELC4JYB+fvhx+Musyotcs39gjyxv64oumX\n"
    "XGYX+YfuP/qWa0MGFxRY8EsvophPThcOH7JeuFGfguRDe5dzXYkaJ4HXEzQ41MvX\n"
    "dJnX6JLiGPmPSwIDAQABo4IBiDCCAYQwDwYDVR0TAQH/BAUwAwIBADALBgNVHQ8E\n"
    "BAMCBeAwFAYDVR0RBA0wC4IJd2ktZmkub3JnMBMGA1UdJQQMMAoGCCsGAQUFBwMB\n"
    "MB0GA1UdDgQWBBScwZzvEAiqVwRJQrLLXQ7gDMv8MDAfBgNVHSMEGDAWgBQLA8I+\n"
    "VKIovT5J3nLxX46rDpdngjAmBgsrBgEEAYK+aAEBAQQXDBVFTkdXaS1GaSBBbGxp\n"
    "YW5jZSBPU1UwgdAGCCsGAQUFBwEMBIHDMIHAoIG9MIG6oIG3MIG0MIGxMIGuFglp\n"
    "bWFnZS9wbmcwUTBPMAsGCWCGSAFlAwQCAQRARDQ0NzRBMjcyQkIwRDAxODJEQjlB\n"
    "RDdDNzdGM0JDQTRDMTM2MUU4QUNFMkVDQzIwOUI3QzI1QkJEQzc2MUQxMDBOFkxo\n"
    "dHRwOi8vd3d3LndpLWZpLm9yZy9zaXRlcy9kZWZhdWx0L2ZpbGVzL3VwbG9hZHMv\n"
    "V0ZBX0NFUlRJRklFRF8zRF9XZWJfTFIucG5nMA0GCSqGSIb3DQEBCwUAA4IBAQCC\n"
    "QMaT+bkDMMfSjHpl62fIRHxWwdyPBo7otQ+GIrSlu+9N2P7n7hwkGQ+7vb6IeZ/H\n"
    "TiWfJXg+nMMF+A1HGxEARjAXaO1RenzA6gx9dv3XHKJiwuTk/Zy0+/KoAk3ln8tg\n"
    "lhxP48LRuJ2Z+biw7ayF6GI1RPrCyq5Bl8e5jiqLcZ5WCz9W6X+e2qAFTQLqDQVR\n"
    "e/ZMI0ASvUG+tM5VxQyWbEWjPjCN+uSoxNxZ/KIU/biuhdOPLxoGtdZkrD6qHv6j\n"
    "WdvWZs0BjtGkZjmusg8UxxZvx9c1zQSR4bUoXNmkVGs3mK52pnC65czHhIzHj5ZG\n"
    "Q8aUClQmwK/wC1dwToz1\n"
    "-----END CERTIFICATE-----\n";

/**
 * @brief TLS provisioning certification private file 
 */
const unsigned char prov_tls_key[] = "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEowIBAAKCAQEAyCFCRmy1hNJpZVFK5VAUILS0ukrHXAlSj6EQ9imSulkp1s1v\n"
    "tNw0iP0Zb6o1G+duMeowUMe1Ww941XBXQamRo5HnzuQ0Je2gcJdV/CxFSZPOGc8W\n"
    "VnQIXC/snYLHZf4yUpgxTmh12wJIRgy8o8d0YjN4OLQeRZVSTW4zzjRCQqrRQjnw\n"
    "bRBhLxwMWVK+W6rG7Jtf7rE1u9grGbicUysPtPQn/B/Sm9EX0y45hTELC4JYB+fv\n"
    "hx+Musyotcs39gjyxv64oumXXGYX+YfuP/qWa0MGFxRY8EsvophPThcOH7JeuFGf\n"
    "guRDe5dzXYkaJ4HXEzQ41MvXdJnX6JLiGPmPSwIDAQABAoIBAA/W6gru4bMAgEz/\n"
    "kSDzJJSuGLvB3WOAbWNmyRPimHVdRz10BwpWf2X0OkYnP4rU11jmAyrxk34AjHzA\n"
    "JnfQ9vDuRF2QnDwAQbmHkMujqVw4cZCDXm47QsohFOYmiqec5di5qPVeuS07UgMR\n"
    "UQDZcXGwRyydvOOe/OOZ/EmuwyvJe2XntKHWuU2XvurWWU2zrT6CrzZiQaZuv/Lq\n"
    "fbrhG/yobnw89YgZ31RqX14RTLVm8glW07mtb0qBiJFL9W9hcU3dL4x8hQJn+nsR\n"
    "xzn/bJ6RvVld2B+rZWvqp9FqChvoNQjnB+KUsGREzEk3lBP6auSWp+WqjoiRVSxT\n"
    "NOQupVECgYEA7UKCAXmjZhJeNf1K2HRF9RliWMk9ueC6eC/4KHQmhUH0QlZ5nKZJ\n"
    "Q0VtjZkaMxndMbyhi3KLcJEp7SHe1qxqQKYTQyfGrh5P5VMqEIlon8HbxeQV1MyJ\n"
    "itXcQ/w0c60LjensyqcG+a/ASShsQlbLUEu4KprKxwjUE7bLoc0L+LkCgYEA1+/4\n"
    "Kn6WO+YOAz41Mr7spRnaIjBidHfYVmP69GuiujxLZt9fJvPVJaDeDEoG0IGUMxAO\n"
    "hNOe3urEY30xyQNlHXVwcq8aWS5xKcOFriWVu7uyLKZTSEwNmGV7WqDof/8ltULd\n"
    "zxqEDqwMC47LvVWAJQIjvK14cSYj8mgl2jaH/iMCgYEAtR0L+OxN22EyIayVMd6w\n"
    "eIVEGdqlD/uI3K5hlR/1N8w9FVbFxtr5Gi0pj+nLoi6gN5NzDewGnYtyod41KVK8\n"
    "WVVtZto6RDhHbRurBKyf/TQS/GE8eREZ/a17Uzp3H8Z9B3wGkjwmivuqoS4GVi3D\n"
    "Tiw/DebT3FPU8KedkongFaECgYAJO3dOyc6+jN14ggZgPw17GZnb8FmlRSJ4vxmR\n"
    "rbBtafqHUXGOBsrMKw6TU+7qpz+g10TnpOyb90miP04LnBuMoOLH5Hip63RnJrbm\n"
    "dTDrr+C0TAAvjSDfrScS/uIx5sTD9THqkuWmgvCY5egMFkW+T1Mb1AbcP5c13AIS\n"
    "TFaQJQKBgA54NtGt3Gm5zvxefPJZSSbTpeIsO0a8Es2IdnVMFYOqzQ/wsLYn6xJO\n"
    "Cjd0yCeauOFsDh4h/7xJ0Jce3Axc0PkFqPkUjXiOKgKQE37A6OYdRwtB6Lae2LvB\n"
    "cOBdXwCCVW1XM3LD1uf90aUOo8oEvyKlPGXOJOM5j7Zy03Ib2kRO\n"
    "-----END RSA PRIVATE KEY-----\n";

/**
 * @brief TLS provisioning Server inforamtion struct
 */
typedef struct prov_tls_svr_config {
    mbedtls_net_context sock_ctx;
    unsigned int local_port;

    // for TLS
    mbedtls_ssl_context *ssl_ctx;
    mbedtls_ssl_config *ssl_conf;
    mbedtls_ctr_drbg_context *ctr_drbg_ctx;
    mbedtls_entropy_context *entropy_ctx;

    mbedtls_x509_crt *ca_cert_crt;
    mbedtls_x509_crt *cert_crt;
    mbedtls_pk_context *pkey_ctx;
    mbedtls_pk_context *pkey_alt_ctx;
} prov_tls_svr_conf_t;

///////////////////////////////////////////////////

/**
 * @brief Global library external functions
 */

/**
 ****************************************************************************************
 * @brief TLS provisioning server configure initialize
 * @param[in] config the TLS server information
 * @return int err_enum_t
 ****************************************************************************************
 */
int app_prov_tls_svr_init_config(prov_tls_svr_conf_t *config);

/**
 ****************************************************************************************
 * @brief TLS provisioning server tcp socket initialize
 * @param[in] config the TLS server information
 * @return int err_enum_t
 ****************************************************************************************
 */
int app_prov_tls_svr_init_socket(prov_tls_svr_conf_t *config);

/**
 ****************************************************************************************
 * @brief TLS provisioning server tcp socket deinitialize
 * @param[in] config the TLS server information
 * @return int err_enum_t
 ****************************************************************************************
 */
int app_prov_tls_svr_deinit_socket(prov_tls_svr_conf_t *config);

/**
 ****************************************************************************************
 * @brief TLS provisioning server session initialize
 * @param[in] config the TLS server information
 * @return int err_enum_t
 ****************************************************************************************
 */
int app_prov_tls_svr_init_ssl(prov_tls_svr_conf_t *config);

/**
 ****************************************************************************************
 * @brief TLS provisioning server sesstion setup
 * @param[in] config the TLS server information
 * @return int err_enum_t
 ****************************************************************************************
 */
int app_prov_tls_svr_setup_ssl(prov_tls_svr_conf_t *config);

/**
 ****************************************************************************************
 * @brief TLS provisioning server session deinitialize
 * @param[in] config the TLS server information
 * @return int err_enum_t
 ****************************************************************************************
 */
int app_prov_tls_svr_deinit_ssl(prov_tls_svr_conf_t *config);

/**
 ****************************************************************************************
 * @brief TLS provisioning server session shutdown
 * @param[in] config the TLS server information
 * @return int err_enum_t
 ****************************************************************************************
 */
int app_prov_tls_svr_shutdown_ssl(prov_tls_svr_conf_t *config);

#endif	// __SUPPORT_WIFI_PROVISIONING__

#endif	// __APP_SOFTAP_PROVISION_H__

/* EOF */
