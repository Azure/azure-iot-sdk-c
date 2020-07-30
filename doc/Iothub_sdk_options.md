# IoTHub C SDK Options

This document describes how to set the options available in the c sdk.

- [Setting an Options](#set_option)
- [IoThub Client Options](#IotHub_option)
- [Transport Options](#transport_option)

<a name="set_option"></a>

## Setting an Option

Setting an option in the c-sdk is dependant on which api set you are using:

```c
// Convience Layer
IOTHUB_CLIENT_RESULT IoTHubClient_SetOption(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const char* optionName, const void* value)

// Single Thread API
IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetOption(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* optionName, const void* value)
```

An example of using a set option:

```c
bool trace_on = true;
IoTHubClient_SetOption(sdk_handle, OPTION_LOG_TRACE, &trace_on);

HTTP_PROXY_OPTIONS http_proxy;
memset(&http_proxy, 0, sizeof(HTTP_PROXY_OPTIONS));
http_proxy.host_address = PROXY_ADDRESS;
http_proxy.port = PROXY_PORT;
DPS_LL_SetOption(handle, OPTION_HTTP_PROXY, &http_proxy);
```

## Available Options

<a name="IotHub_option"></a>

### IoThub_Client

| Option Name                       | Option Define                   | Value Type         | Description
|-----------------------------------|---------------------------------|--------------------|-------------------------------
| `"messageTimeout"`              | OPTION_MESSAGE_TIMEOUT         | tickcounter_ms_t*  | (DEPRECATED) Timeout used for message on the message queue
| `"blob_upload_timeout_secs"`  | OPTION_BLOB_UPLOAD_TIMEOUT_SECS | size_t*            | Timeout in seconds of blob uploads
| `"product_info"`                | OPTION_PRODUCT_INFO             | const char*        | User defined Product identifier sent to the IoThub service
| `"TrustedCerts"`                | OPTION_TRUSTED_CERT             | const char*        | Azure Server certificate used to validate TLS connection to iothub
| `"retry_interval_sec"`          | OPTION_RETRY_INTERVAL_SEC       |  int*              | Amount of seconds between retries when using the interval retry policy

<a name="transport_option"></a>

### Standard Transport

| Option Name            | Option Define             | Value Type         | Description
|------------------------|---------------------------|--------------------|-------------------------------
| `"logtrace"`           | OPTION_LOG_TRACE          | bool* value        | Turn on and off log tracing for the transport
| `"sas_token_lifetime"` | OPTION_SAS_TOKEN_LIFETIME | size_t* value    | Length of time in seconds used for lifetime of sas token.
| `"x509certificate"`    | OPTION_X509_CERT          | const char*        | Sets an RSA x509 certificate used for connection authentication
| `"x509privatekey"`     | OPTION_X509_PRIVATE_KEY   | const char*        | Sets the private key for the RSA x509 certificate
| `"x509EccCertificate"` | OPTION_X509_ECC_CERT      | const char*        | Sets the ECC x509 certificate used for connection authentication
| `"x509EccAliasKey"`    | OPTION_X509_ECC_KEY       | const char*        | Sets the private key for the ECC x509 certificate
| `"proxy_data"`         | OPTION_HTTP_PROXY         | [HTTP_PROXY_OPTIONS*][http-proxy-object]| Http proxy data object used for proxy connection to IoTHub
| `"tls_version"`         | OPTION_TLS_VERSION         | int*            | TLS version to use for openssl, 10 for version 1.0, 11 for version 1.1, 12 for version 1.2

### MQTT Transport

| Option Name               | Option Define                 | Value Type         | Description
|---------------------------|-------------------------------|--------------------|-------------------------------
| `"keepalive"`             | OPTION_KEEP_ALIVE             | int*               | Length of time to send `Keep Alives` to service for D2C Messages
| `"auto_url_encode_decode"`| OPTION_AUTO_URL_ENCODE_DECODE | bool*              | Turn on and off automatic URL Encoding and Decoding.

### AMQP Transport

| Option Name                  | Option Define                   | Value Type        | Description
|------------------------------|---------------------------------|-------------------|-------------------------------
| `"cbs_request_timeout"`      | OPTION_CBS_REQUEST_TIMEOUT      | `size_t`* value   | Amount of seconds to wait for a cbs request to complete
| `"sas_token_refresh_time"`   | OPTION_SAS_TOKEN_REFRESH_TIME   | `size_t`* value   | Frequency in seconds that the SAS token is refreshed
| `"event_send_timeout_secs"`  | OPTION_EVENT_SEND_TIMEOUT_SECS  | `size_t`* value   | Amount of seconds to wait for telemetry message to complete
| `"c2d_keep_alive_freq_secs"` | OPTION_C2D_KEEP_ALIVE_FREQ_SECS | `size_t`* value   | Informs service of maximum period the client waits for keep-alive message

### HTTP Tansport

| Option Name                  | Option Define                   | Value Type        | Description
|------------------------------|---------------------------------|-------------------|-------------------------------
| `"Batching"`                 | OPTION_BATCHING                 | `bool`* value     | Turn on and off message batching
| `"MinimumPollingTime"`       | OPTION_MIN_POLLING_TIME         | `unsigned int`* value     | Minimum time in seconds allowed between 2 consecutive GET issues to the service
| `"timeout"`                  | OPTION_HTTP_TIMEOUT             | `long`* value     | When using curl the amount of time before the request times out, defaults to 242 seconds.

### Advanced Compilation Options

We recommend leaving the following settings at their defaults. Tuning them may allow optimizations for specific devices or scenarios but could also negatively impact RAM or EEPROM usage.
The options are presented only as compilation flags and must be appended to the CMake `compileOption_C` setting:

| Option Name                  | Option Define                                           | Description
|------------------------------|---------------------------------------------------------|-------------------------------------------------------------
| `"XIO Receive Buffer"`       | `-DcompileOption_C="-DXIO_RECEIVE_BUFFER_SIZE=<value>"` | Configure the internal XIO receive buffer.

## Additional notes

### Batching and IoTHub Client SDK

Batching is the ability of a protocol to send multiple messages in one payload, rather than one at a time.  This can result in less overhead, especially when sending multiple, small messages.  This SDK supports various levels of batching when using IoTHubClient_SendEventAsync / IoTHubClient_LL_SendEventAsync.

- AMQP uses batching always.

- HTTP can optionally enable batching, using the "Batching" option referenced above.

- MQTT does not have a batching option.

None of the protocols has a windowing or Nagling concept; e.g. they do NOT wait a certain amount of time to attempt to queue up multiple messages to put into a single batch.  Instead they just batch whatever is on the to-send queue.  For customers using the lower-layer protocols (LL), they can force batching via

IoTHubClient_LL_SendEventAsync(msg1)
IoTHubClient_LL_SendEventAsync(msg2)
IoTHubClient_LL_DoWork()

[http-proxy-object]: https://github.com/Azure/azure-c-shared-utility/blob/506288cecb9ee4a205fa221dc4fd2e69a7ddaa7e/inc/azure_c_shared_utility/shared_util_options.h



