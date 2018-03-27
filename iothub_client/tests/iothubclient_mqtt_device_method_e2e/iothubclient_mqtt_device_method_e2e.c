// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iothubclient_common_device_method_e2e.h"
#include "testrunnerswitcher.h"
#include "iothubtransportmqtt.h"
#include "iothubtransportmqtt_websockets.h"
#include "iothub_devicemethod.h"


static TEST_MUTEX_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(iothubclient_mqtt_device_method_e2e)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        device_method_e2e_init();
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        device_method_e2e_deinit();
        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_FUNCTION_CLEANUP(TestFunctionCleanup)
    {
        device_method_function_cleanup();
    }

    //
    // MQTT tests.
    //
    TEST_FUNCTION(IotHub_Mqtt_Method_Call_With_String_sas)
    {
        device_method_e2e_method_call_with_string_sas(MQTT_Protocol);
    }

    TEST_FUNCTION(IotHub_Mqtt_Method_Call_With_Double_Quoted_Json_sas)
    {
        device_method_e2e_method_call_with_double_quoted_json_sas(MQTT_Protocol);
    }

    TEST_FUNCTION(IotHub_Mqtt_Method_Call_With_Empty_Json_Object_sas)
    {
        device_method_e2e_method_call_with_empty_json_object_sas(MQTT_Protocol);
    }

    TEST_FUNCTION(IotHub_Mqtt_Method_Call_With_Null_sas)
    {
        device_method_e2e_method_call_with_null_sas(MQTT_Protocol);
    }

    TEST_FUNCTION(IotHub_Mqtt_Method_Call_With_Embedded_Double_Quote_sas)
    {
        device_method_e2e_method_call_with_embedded_double_quote_sas(MQTT_Protocol);
    }

    TEST_FUNCTION(IotHub_Mqtt_Method_Call_With_Embedded_Single_Quote_sas)
    {
        device_method_e2e_method_call_with_embedded_single_quote_sas(MQTT_Protocol);
    }

#ifndef __APPLE__
#ifndef DONT_USE_UPLOADTOBLOB
    TEST_FUNCTION(IotHub_Mqtt_Method_Call_With_UploadBlob_sas)
    {
        device_method_e2e_method_calls_upload_sas(MQTT_Protocol);
    }

    TEST_FUNCTION(IotHub_Mqtt_Method_Call_With_UploadBlob_x509)
    {
        device_method_e2e_method_calls_upload_x509(MQTT_Protocol);
    }
#endif

    TEST_FUNCTION(IotHub_Mqtt_Method_Call_With_String_x509)
    {
        device_method_e2e_method_call_with_string_x509(MQTT_Protocol);
    }

    TEST_FUNCTION(IotHub_Mqtt_Method_Call_With_Double_Quoted_Json_x509)
    {
        device_method_e2e_method_call_with_double_quoted_json_x509(MQTT_Protocol);
    }

    TEST_FUNCTION(IotHub_Mqtt_Method_Call_With_Empty_Json_Object_x509)
    {
        device_method_e2e_method_call_with_empty_json_object_x509(MQTT_Protocol);
    }

    TEST_FUNCTION(IotHub_Mqtt_Method_Call_With_Null_x509)
    {
        device_method_e2e_method_call_with_null_x509(MQTT_Protocol);
    }

    TEST_FUNCTION(IotHub_Mqtt_Method_Call_With_Embedded_Double_Quote_x509)
    {
        device_method_e2e_method_call_with_embedded_double_quote_x509(MQTT_Protocol);
    }

    TEST_FUNCTION(IotHub_Mqtt_Method_Call_With_Embedded_Single_Quote_x509)
    {
        device_method_e2e_method_call_with_embedded_single_quote_x509(MQTT_Protocol);
    }

    //
    // MQTT_WS tests.  Only test small subset.
    //
    TEST_FUNCTION(IotHub_Mqtt_Ws_Method_Call_With_String_x509)
    {
        device_method_e2e_method_call_with_string_x509(MQTT_WebSocket_Protocol);
    }
#endif

    TEST_FUNCTION(IotHub_Mqtt_Ws_Method_Call_With_String_sas)
    {
        device_method_e2e_method_call_with_string_sas(MQTT_WebSocket_Protocol);
    }

END_TEST_SUITE(iothubclient_mqtt_device_method_e2e)

