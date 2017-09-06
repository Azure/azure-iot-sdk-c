// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_hub_modules/dps_transport_mqtt_client.h"
#include "azure_hub_modules/dps_transport_mqtt_common.h"
#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "azure_c_shared_utility/platform.h"

#define DPS_MQTT_PORT_NUM               8883

static XIO_HANDLE mqtt_transport_io(const char* fqdn, const HTTP_PROXY_OPTIONS* proxy_info)
{
    XIO_HANDLE result;
    HTTP_PROXY_IO_CONFIG proxy_config;
    TLSIO_CONFIG tls_io_config;

    memset(&tls_io_config, 0, sizeof(TLSIO_CONFIG));
    tls_io_config.hostname = fqdn;
    tls_io_config.port = DPS_MQTT_PORT_NUM;
    if (proxy_info != NULL)
    {
        /* Codes_DPS_TRANSPORT_MQTT_CLIENT_07_012: [ If proxy_info is not NULL, amqp_transport_io shall construct a HTTP_PROXY_IO_CONFIG object and assign it to TLSIO_CONFIG underlying_io_parameters ] */
        proxy_config.hostname = tls_io_config.hostname;
        proxy_config.port = DPS_MQTT_PORT_NUM;
        proxy_config.proxy_hostname = proxy_info->host_address;
        proxy_config.proxy_port = proxy_info->port;
        proxy_config.username = proxy_info->username;
        proxy_config.password = proxy_info->password;

        tls_io_config.underlying_io_interface = http_proxy_io_get_interface_description();
        tls_io_config.underlying_io_parameters = &proxy_config;
    }

    const IO_INTERFACE_DESCRIPTION* tlsio_interface = platform_get_default_tlsio();
    if (tlsio_interface == NULL)
    {
        /* Codes_DPS_TRANSPORT_MQTT_CLIENT_07_013: [ If any failure is encountered amqp_transport_io shall return NULL ]*/
        LogError("platform_get_default_tlsio return NULL IO Interface");
        result = NULL;
    }
    else
    {
        result = xio_create(tlsio_interface, &tls_io_config);
        if (result == NULL)
        {
            /* Codes_DPS_TRANSPORT_MQTT_CLIENT_07_013: [ If any failure is encountered amqp_transport_io shall return NULL ]*/
            LogError("failed calling xio_create on underlying io");
            result = NULL;
        }
        else
        {
            // DPS requires tls 1.2
            int tls_version = 12;
            xio_setoption(result, "tls_version", &tls_version);
        }
    }
    /* Codes_DPS_TRANSPORT_MQTT_CLIENT_07_014: [ On success mqtt_transport_io shall return allocated XIO_HANDLE. ] */
    return result;
}

DPS_TRANSPORT_HANDLE dps_transport_mqtt_create(const char* uri, DPS_HSM_TYPE type, const char* scope_id, const char* registration_id, const char* dps_api_version)
{
    /* Codes_DPS_TRANSPORT_MQTT_CLIENT_07_001: [ dps_transport_mqtt_create shall call the dps_transport_common_mqtt_create function with mqtt_transport_io transport IO estabishment. ] */
    return dps_transport_common_mqtt_create(uri, type, scope_id, registration_id, dps_api_version, mqtt_transport_io);
}

void dps_transport_mqtt_destroy(DPS_TRANSPORT_HANDLE handle)
{
    /* Codes_DPS_TRANSPORT_MQTT_CLIENT_07_002: [ dps_transport_mqtt_destroy shall invoke the dps_transport_common_mqtt_destroy method ] */
    dps_transport_common_mqtt_destroy(handle);
}

int dps_transport_mqtt_open(DPS_TRANSPORT_HANDLE handle, BUFFER_HANDLE ek, BUFFER_HANDLE srk, DPS_TRANSPORT_REGISTER_DATA_CALLBACK data_callback, void* user_ctx, DPS_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx)
{
    /* Codes_DPS_TRANSPORT_MQTT_CLIENT_07_003: [ dps_transport_mqtt_open shall invoke the dps_transport_common_mqtt_open method ] */
    return dps_transport_common_mqtt_open(handle, ek, srk, data_callback, user_ctx, status_cb, status_ctx);
}

int dps_transport_mqtt_close(DPS_TRANSPORT_HANDLE handle)
{
    /* Codes_DPS_TRANSPORT_MQTT_CLIENT_07_004: [ dps_transport_mqtt_close shall invoke the dps_transport_common_mqtt_close method ] */
    return dps_transport_common_mqtt_close(handle);
}

int dps_transport_mqtt_register_device(DPS_TRANSPORT_HANDLE handle, DPS_TRANSPORT_CHALLENGE_CALLBACK reg_challenge_cb, void* user_ctx, DPS_TRANSPORT_JSON_PARSE json_parse_cb, void* json_ctx)
{
    /* Codes_DPS_TRANSPORT_MQTT_CLIENT_07_005: [ dps_transport_mqtt_register_device shall invoke the dps_transport_common_mqtt_register_device method ] */
    return dps_transport_common_mqtt_register_device(handle, reg_challenge_cb, user_ctx, json_parse_cb, json_ctx);
}

int dps_transport_mqtt_get_operation_status(DPS_TRANSPORT_HANDLE handle)
{
    /* Codes_DPS_TRANSPORT_MQTT_CLIENT_07_006: [ dps_transport_mqtt_get_operation_status shall invoke the dps_transport_common_mqtt_get_operation_status method ] */
    return dps_transport_common_mqtt_get_operation_status(handle);
}

void dps_transport_mqtt_dowork(DPS_TRANSPORT_HANDLE handle)
{
    /* Codes_DPS_TRANSPORT_MQTT_CLIENT_07_007: [ dps_transport_mqtt_dowork shall invoke the dps_transport_common_mqtt_dowork method ] */
    dps_transport_common_mqtt_dowork(handle);
}

int dps_transport_mqtt_set_trace(DPS_TRANSPORT_HANDLE handle, bool trace_on)
{
    /* Codes_DPS_TRANSPORT_MQTT_CLIENT_07_008: [ dps_transport_mqtt_set_trace shall invoke the dps_transport_common_mqtt_set_trace method ] */
    return dps_transport_common_mqtt_set_trace(handle, trace_on);
}

static int dps_transport_mqtt_x509_cert(DPS_TRANSPORT_HANDLE handle, const char* certificate, const char* private_key)
{
    /* Codes_DPS_TRANSPORT_MQTT_CLIENT_07_009: [ dps_transport_mqtt_x509_cert shall invoke the dps_trans_common_mqtt_x509_cert method ]*/
    return dps_transport_common_mqtt_x509_cert(handle, certificate, private_key);
}

static int dps_transport_mqtt_set_trusted_cert(DPS_TRANSPORT_HANDLE handle, const char* certificate)
{
    /* Codes_DPS_TRANSPORT_MQTT_CLIENT_07_010: [ dps_transport_mqtt_set_trusted_cert shall invoke the dps_transport_common_mqtt_set_trusted_cert method ] */
    return dps_transport_common_mqtt_set_trusted_cert(handle, certificate);
}

static int dps_transport_mqtt_set_proxy(DPS_TRANSPORT_HANDLE handle, const HTTP_PROXY_OPTIONS* proxy_options)
{
    /* Codes_DPS_TRANSPORT_MQTT_CLIENT_07_011: [ dps_transport_mqtt_set_proxy shall invoke the dps_transport_common_mqtt_set_proxy method ] */
    return dps_transport_common_mqtt_set_proxy(handle, proxy_options);
}

static DPS_TRANSPORT_PROVIDER dps_mqtt_func = 
{
    dps_transport_mqtt_create,
    dps_transport_mqtt_destroy,
    dps_transport_mqtt_open,
    dps_transport_mqtt_close,
    dps_transport_mqtt_register_device,
    dps_transport_mqtt_get_operation_status,
    dps_transport_mqtt_dowork,
    dps_transport_mqtt_set_trace,
    dps_transport_mqtt_x509_cert,
    dps_transport_mqtt_set_trusted_cert,
    dps_transport_mqtt_set_proxy
};

const DPS_TRANSPORT_PROVIDER* DPS_MQTT_Protocol(void)
{
    return &dps_mqtt_func;
}
