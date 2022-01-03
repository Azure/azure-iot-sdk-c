// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <limits.h>
#include "internal/iothubtransport_amqp_connection.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_uamqp_c/sasl_mechanism.h"
#include "azure_uamqp_c/saslclientio.h"
#include "azure_uamqp_c/sasl_mssbcbs.h"
#include "azure_uamqp_c/connection.h"
#include "azure_c_shared_utility/xlogging.h"

#define RESULT_OK                            0
#define DEFAULT_INCOMING_WINDOW_SIZE         UINT_MAX
#define DEFAULT_OUTGOING_WINDOW_SIZE         100
#define SASL_IO_OPTION_LOG_TRACE             "logtrace"
#define DEFAULT_UNIQUE_ID_LENGTH             40

typedef struct AMQP_CONNECTION_INSTANCE_TAG
{
    STRING_HANDLE iothub_fqdn;
    XIO_HANDLE underlying_io_transport;
    CBS_HANDLE cbs_handle;
    CONNECTION_HANDLE connection_handle;
    SESSION_HANDLE session_handle;
    XIO_HANDLE sasl_io;
    SASL_MECHANISM_HANDLE sasl_mechanism;
    bool has_cbs;
    bool has_sasl_mechanism;
    bool is_trace_on;
    AMQP_CONNECTION_STATE current_state;
    ON_AMQP_CONNECTION_STATE_CHANGED on_state_changed_callback;
    const void* on_state_changed_context;
    uint32_t svc2cl_keep_alive_timeout_secs;
    double cl2svc_keep_alive_send_ratio;
} AMQP_CONNECTION_INSTANCE;


MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(AMQP_CONNECTION_STATE, AMQP_CONNECTION_STATE_VALUES);


static int create_sasl_components(AMQP_CONNECTION_INSTANCE* instance)
{
    int result;
    SASL_MECHANISM_HANDLE sasl_mechanism;
    XIO_HANDLE sasl_io;

    if ((sasl_mechanism = saslmechanism_create(saslmssbcbs_get_interface(), NULL)) == NULL)
    {
        LogError("Failed creating the SASL mechanism (saslmechanism_create failed)");
        result = MU_FAILURE;
    }
    else
    {
        SASLCLIENTIO_CONFIG sasl_client_config;
        sasl_client_config.sasl_mechanism = sasl_mechanism;
        sasl_client_config.underlying_io = instance->underlying_io_transport;

        if ((sasl_io = xio_create(saslclientio_get_interface_description(), &sasl_client_config)) == NULL)
        {
            LogError("Failed creating the SASL I/O (xio_create failed)");
            saslmechanism_destroy(sasl_mechanism);
            result = MU_FAILURE;
        }
        else if (xio_setoption(sasl_io, SASL_IO_OPTION_LOG_TRACE, (const void*)&instance->is_trace_on) != RESULT_OK)
        {
            LogError("Failed setting the SASL I/O logging trace option (xio_setoption failed)");
            xio_destroy(sasl_io);
            saslmechanism_destroy(sasl_mechanism);
            result = MU_FAILURE;
        }
        else
        {
            instance->sasl_mechanism = sasl_mechanism;
            instance->sasl_io = sasl_io;
            result = RESULT_OK;
        }
    }

    return result;
}

static void update_state(AMQP_CONNECTION_INSTANCE* instance, AMQP_CONNECTION_STATE new_state)
{
    if (new_state != instance->current_state)
    {
        AMQP_CONNECTION_STATE previous_state = instance->current_state;
        instance->current_state = new_state;

        if (instance->on_state_changed_callback != NULL)
        {
            instance->on_state_changed_callback(instance->on_state_changed_context, previous_state, new_state);
        }
    }
}

static void on_connection_io_error(void* context)
{
    update_state((AMQP_CONNECTION_INSTANCE*)context, AMQP_CONNECTION_STATE_ERROR);
}

static void on_connection_state_changed(void* context, CONNECTION_STATE new_connection_state, CONNECTION_STATE previous_connection_state)
{
    (void)previous_connection_state;

    AMQP_CONNECTION_INSTANCE* instance = (AMQP_CONNECTION_INSTANCE*)context;

    if (new_connection_state == CONNECTION_STATE_START)
    {
        // connection is using x509 authentication.
        // At this point uamqp's connection only raises CONNECTION_STATE_START when using X509 auth.
        // So that should be all we expect to consider the amqp_connection_handle opened.
        if (instance->has_cbs == false || instance->has_sasl_mechanism == false)
        {
            update_state(instance, AMQP_CONNECTION_STATE_OPENED);
        }
    }
    else if (new_connection_state == CONNECTION_STATE_OPENED)
    {
        update_state(instance, AMQP_CONNECTION_STATE_OPENED);
    }
    else if (new_connection_state == CONNECTION_STATE_END)
    {
        update_state(instance, AMQP_CONNECTION_STATE_CLOSED);
    }
    else if (new_connection_state == CONNECTION_STATE_ERROR || new_connection_state == CONNECTION_STATE_DISCARDING)
    {
        update_state(instance, AMQP_CONNECTION_STATE_ERROR);
    }
}

static void on_cbs_open_complete(void* context, CBS_OPEN_COMPLETE_RESULT open_complete_result)
{
    (void)open_complete_result;
    if (open_complete_result != CBS_OPEN_OK)
    {
        LogError("CBS open failed");
        update_state((AMQP_CONNECTION_INSTANCE*)context, AMQP_CONNECTION_STATE_ERROR);
    }
}

static void on_cbs_error(void* context)
{
    (void)context;
    LogError("CBS Error occurred");
    update_state((AMQP_CONNECTION_INSTANCE*)context, AMQP_CONNECTION_STATE_ERROR);
}

static int create_connection_handle(AMQP_CONNECTION_INSTANCE* instance)
{
    int result;
    char* unique_container_id = NULL;
    XIO_HANDLE connection_io_transport;

    if (instance->sasl_io != NULL)
    {
        connection_io_transport = instance->sasl_io;
    }
    else
    {
        connection_io_transport = instance->underlying_io_transport;
    }

    if ((unique_container_id = (char*)malloc(sizeof(char) * DEFAULT_UNIQUE_ID_LENGTH + 1)) == NULL)
    {
        result = __LINE__;
        LogError("Failed creating the AMQP connection (failed creating unique ID container)");
    }
    else
    {
        memset(unique_container_id, 0, sizeof(char) * DEFAULT_UNIQUE_ID_LENGTH + 1);

        if (UniqueId_Generate(unique_container_id, DEFAULT_UNIQUE_ID_LENGTH) != UNIQUEID_OK)
        {
            result = MU_FAILURE;
            LogError("Failed creating the AMQP connection (UniqueId_Generate failed)");
        }
        else if ((instance->connection_handle = connection_create2(connection_io_transport, STRING_c_str(instance->iothub_fqdn), unique_container_id, NULL, NULL, on_connection_state_changed, (void*)instance, on_connection_io_error, (void*)instance)) == NULL)
        {
            result = MU_FAILURE;
            LogError("Failed creating the AMQP connection (connection_create2 failed)");
        }
        else if (connection_set_idle_timeout(instance->connection_handle, 1000 * instance->svc2cl_keep_alive_timeout_secs) != RESULT_OK)
        {
            result = MU_FAILURE;
            LogError("Failed creating the AMQP connection (connection_set_idle_timeout failed)");
        }
        else if (connection_set_remote_idle_timeout_empty_frame_send_ratio(instance->connection_handle, instance->cl2svc_keep_alive_send_ratio) != RESULT_OK)
        {
            result = MU_FAILURE;
            LogError("Failed creating the AMQP connection (connection_set_remote_idle_timeout_empty_frame_send_ratio)");
        }
        else
        {
            connection_set_trace(instance->connection_handle, instance->is_trace_on);

            result = RESULT_OK;
        }
    }

    if (unique_container_id != NULL)
    {
        free(unique_container_id);
    }

    return result;
}

static int create_session_handle(AMQP_CONNECTION_INSTANCE* instance)
{
    int result;

    if ((instance->session_handle = session_create(instance->connection_handle, NULL, NULL)) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed creating the AMQP connection (connection_create2 failed)");
    }
    else
    {
        if (session_set_incoming_window(instance->session_handle, (uint32_t)DEFAULT_INCOMING_WINDOW_SIZE) != 0)
        {
            LogError("Failed to set the AMQP session incoming window size.");
        }

        if (session_set_outgoing_window(instance->session_handle, DEFAULT_OUTGOING_WINDOW_SIZE) != 0)
        {
            LogError("Failed to set the AMQP session outgoing window size.");
        }

        result = RESULT_OK;
    }

    return result;
}

static int create_cbs_handle(AMQP_CONNECTION_INSTANCE* instance)
{
    int result;

    if ((instance->cbs_handle = cbs_create(instance->session_handle)) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed to create the CBS connection.");
    }
    else if (cbs_open_async(instance->cbs_handle, on_cbs_open_complete, instance, on_cbs_error, instance) != RESULT_OK)
    {
        result = MU_FAILURE;
        LogError("Failed to open the connection with CBS.");
    }
    else
    {
        result = RESULT_OK;
    }

    return result;
}


// Public APIS:

void amqp_connection_destroy(AMQP_CONNECTION_HANDLE conn_handle)
{
    if (conn_handle != NULL)
    {
        AMQP_CONNECTION_INSTANCE* instance = (AMQP_CONNECTION_INSTANCE*)conn_handle;

        if (instance->cbs_handle != NULL)
        {
            cbs_destroy(instance->cbs_handle);
        }

        if (instance->session_handle != NULL)
        {
            session_destroy(instance->session_handle);
        }

        if (instance->connection_handle != NULL)
        {
            connection_destroy(instance->connection_handle);
        }

        if (instance->sasl_io != NULL)
        {
            xio_destroy(instance->sasl_io);
        }

        if (instance->sasl_mechanism != NULL)
        {
            saslmechanism_destroy(instance->sasl_mechanism);
        }

        if (instance->iothub_fqdn != NULL)
        {
            STRING_delete(instance->iothub_fqdn);
        }

        free(instance);
    }
}

AMQP_CONNECTION_HANDLE amqp_connection_create(AMQP_CONNECTION_CONFIG* config)
{
    AMQP_CONNECTION_HANDLE result;

    if (config == NULL)
    {
        result = NULL;
        LogError("amqp_connection_create failed (config is NULL)");
    }
    else if (config->iothub_host_fqdn == NULL)
    {
        result = NULL;
        LogError("amqp_connection_create failed (config->iothub_host_fqdn is NULL)");
    }
    else if (config->underlying_io_transport == NULL)
    {
        result = NULL;
        LogError("amqp_connection_create failed (config->underlying_io_transport is NULL)");
    }
    else
    {
        AMQP_CONNECTION_INSTANCE* instance;

        if ((instance = (AMQP_CONNECTION_INSTANCE*)malloc(sizeof(AMQP_CONNECTION_INSTANCE))) == NULL)
        {
            result = NULL;
            LogError("amqp_connection_create failed (malloc failed)");
        }
        else
        {
            memset(instance, 0, sizeof(AMQP_CONNECTION_INSTANCE));

            if ((instance->iothub_fqdn = STRING_construct(config->iothub_host_fqdn)) == NULL)
            {
                result = NULL;
                LogError("amqp_connection_create failed (STRING_construct failed)");
            }
            else
            {
                instance->underlying_io_transport = config->underlying_io_transport;
                instance->is_trace_on = config->is_trace_on;
                instance->on_state_changed_callback = config->on_state_changed_callback;
                instance->on_state_changed_context = config->on_state_changed_context;
                instance->has_sasl_mechanism = config->create_sasl_io;
                instance->has_cbs = config->create_cbs_connection;

                instance->svc2cl_keep_alive_timeout_secs = (uint32_t)config->svc2cl_keep_alive_timeout_secs;
                instance->cl2svc_keep_alive_send_ratio = (double)config->cl2svc_keep_alive_send_ratio;

                instance->current_state = AMQP_CONNECTION_STATE_CLOSED;

                if ((config->create_sasl_io || config->create_cbs_connection) && create_sasl_components(instance) != RESULT_OK)
                {
                    result = NULL;
                    LogError("amqp_connection_create failed (failed creating the SASL components)");
                }
                else if (create_connection_handle(instance) != RESULT_OK)
                {
                    result = NULL;
                    LogError("amqp_connection_create failed (failed creating the AMQP connection)");
                }
                else if (create_session_handle(instance) != RESULT_OK)
                {
                    result = NULL;
                    LogError("amqp_connection_create failed (failed creating the AMQP session)");
                }
                else if (config->create_cbs_connection && create_cbs_handle(instance) != RESULT_OK)
                {
                    result = NULL;
                    LogError("amqp_connection_create failed (failed creating the CBS handle)");
                }
                else
                {
                    result = (AMQP_CONNECTION_HANDLE)instance;
                }
            }

            if (result == NULL)
            {
                amqp_connection_destroy((AMQP_CONNECTION_HANDLE)instance);
            }
        }
    }

    return result;
}

void amqp_connection_do_work(AMQP_CONNECTION_HANDLE conn_handle)
{
    if (conn_handle != NULL)
    {
        AMQP_CONNECTION_INSTANCE* instance = (AMQP_CONNECTION_INSTANCE*)conn_handle;

        connection_dowork(instance->connection_handle);
    }
}

int amqp_connection_get_session_handle(AMQP_CONNECTION_HANDLE conn_handle, SESSION_HANDLE* session_handle)
{
    int result;

    if (conn_handle == NULL)
    {
        result = MU_FAILURE;
        LogError("amqp_connection_get_session_handle failed (conn_handle is NULL)");
    }
    else if (session_handle == NULL)
    {
        result = MU_FAILURE;
        LogError("amqp_connection_get_session_handle failed (session_handle is NULL)");
    }
    else
    {
        AMQP_CONNECTION_INSTANCE* instance = (AMQP_CONNECTION_INSTANCE*)conn_handle;

        *session_handle = instance->session_handle;

        result = RESULT_OK;
    }

    return result;
}

int amqp_connection_get_cbs_handle(AMQP_CONNECTION_HANDLE conn_handle, CBS_HANDLE* cbs_handle)
{
    int result;

    if (conn_handle == NULL)
    {
        result = MU_FAILURE;
        LogError("amqp_connection_get_cbs_handle failed (conn_handle is NULL)");
    }
    else if (cbs_handle == NULL)
    {
        result = MU_FAILURE;
        LogError("amqp_connection_get_cbs_handle failed (parameter cbs_handle is NULL)");
    }
    else
    {
        AMQP_CONNECTION_INSTANCE* instance = (AMQP_CONNECTION_INSTANCE*)conn_handle;

        if (instance->cbs_handle == NULL)
        {
            result = MU_FAILURE;
            LogError("amqp_connection_get_cbs_handle failed (there is not a cbs_handle to be returned)");
        }
        else
        {
            *cbs_handle = instance->cbs_handle;

            result = RESULT_OK;
        }
    }

    return result;
}

int amqp_connection_set_logging(AMQP_CONNECTION_HANDLE conn_handle, bool is_trace_on)
{
    int result;

    if (conn_handle == NULL)
    {
        result = MU_FAILURE;
        LogError("amqp_connection_set_logging failed (conn_handle is NULL)");
    }
    else
    {
        AMQP_CONNECTION_INSTANCE* instance = (AMQP_CONNECTION_INSTANCE*)conn_handle;

        instance->is_trace_on = is_trace_on;

        if (instance->sasl_io != NULL &&
            xio_setoption(instance->sasl_io, SASL_IO_OPTION_LOG_TRACE, (const void*)&instance->is_trace_on) != RESULT_OK)
        {
            result = MU_FAILURE;
            LogError("amqp_connection_set_logging failed (xio_setoption() failed)");
        }
        else
        {
            connection_set_trace(instance->connection_handle, instance->is_trace_on);

            result = RESULT_OK;
        }
    }

    return result;
}
