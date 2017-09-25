# IoTHub C SDK Options

This document describes the options available in the c sdk and how to use the.

- [Setting an Options](#set_option)
- [IoThub Client Options](#IotHub_option)
- [Transport Options](#transport_option)

<a name="set_option"></a>

## Setting an Option

Setting and option in the c-sdk is dependant on which api set you are using.  If using the convience layer you would set the options in the api as follows:

```c
IOTHUB_CLIENT_RESULT IoTHubClient_SetOption(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const char* optionName, const void* value)

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetOption(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* optionName, const void* value)
```

An example of using a set option:

```c
bool trace_on = true;
IoTHubClient_SetOption(sdk_handle, OPTION_LOG_TRACE, &trace_on);
```

## Available Option

<a name="IotHub_option"></a>

### IoThub_Client

| Option Name        | Option Define              | Value Type         | Description                  |
|--------------------|----------------------------|--------------------|-------------------------------|
| `"messageTimeout"` | OPTION_MESSAGE_TIMEOUT     | tickcounter_ms_t*  | Timeout used for message      |
| `"product_info"`   | OPTION_PRODUCT_INFO        | const char*        | Product information sent to the IoThub service on connection
| `"TrustedCerts"`   | OPTION_TRUSTED_CERT        | const char*        | Azure Server certificate used to validate TLS connection to iothub|

<a name="transport_option"></a>

### Standard Transport

| Option Name            | Option Define             | Value Type         | Description                  |
|------------------------|---------------------------|--------------------|-------------------------------|
| `"logtrace"`           | OPTION_LOG_TRACE          | bool* value         | Turn on and off log trace       |
| `"sas_token_lifetime"` | OPTION_SAS_TOKEN_LIFETIME | `size_t`* value      | Length of time in seconds used for lifetime of sas token. |
| `"x509certificate"`    | OPTION_X509_CERT          | const char*        | Sets the x509 certificate used for connection authentication
| `"x509privatekey"`     | OPTION_X509_PRIVATE_KEY   | const char*        | Sets the private key for the x509 certificate
| `"x509EccCertificate"` | OPTION_X509_ECC_CERT      | const char*        | Sets the ECC x509 certificate used for connection authentication
| `"x509EccAliasKey"`    | OPTION_X509_ECC_KEY       | const char*        | Sets the private key for the ECC x509 certificate
| `"proxy_data"`         | OPTION_HTTP_PROXY         | [HTTP_PROXY_OPTIONS*][http-proxy-object]| Http proxy data object used for proxy connection to IoTHub

### MQTT Transport

| Option Name            | Option Define             | Value Type         | Description                  |
|------------------------|---------------------------|--------------------|-------------------------------|
| `"keepalive"`          | OPTION_KEEP_ALIVE         | int*               | Length of time to send keep Alives to service

### AMQP Transport

| Option Name            | Option Define                 | Value Type         | Description                  |
|------------------------|-------------------------------|--------------------|-------------------------------|
| `"cbs_request_timeout"`| OPTION_CBS_REQUEST_TIMEOUT    | `size_t`* value    | Length of time for CBS token to expire
| `"sas_token_refresh_time"`| OPTION_SAS_TOKEN_REFRESH_TIME | `size_t`* value    | Length of time to refresh the sas token
| `"event_send_timeout_secs"`| OPTION_EVENT_SEND_TIMEOUT_SECS | `size_t`* value    | 
| `"c2d_keep_alive_freq_secs"` | OPTION_C2D_KEEP_ALIVE_FREQ_SECS | `size_t`* value   | Informs service of maximum period the client waits for keep-alive message |

[http-proxy-object]: https://github.com/Azure/azure-c-shared-utility/blob/506288cecb9ee4a205fa221dc4fd2e69a7ddaa7e/inc/azure_c_shared_utility/shared_util_options.h