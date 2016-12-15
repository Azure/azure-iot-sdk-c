# IotHubServiceClient Requirements

## Overview

IotHubServiceClient provides functionality for creating authentication info from IoTHub connection string.

## References

[IoT Hub SDK.doc](https://microsoft.sharepoint.com/teams/Azure_IoT/_layouts/15/WopiFrame.aspx?sourcedoc={9A552E4B-EC00-408F-AE9A-D8C2C37E904F}&file=IoT%20Hub%20SDK.docx&action=default)
[Connection string parser requirement]()

## Exposed API

```c
extern const char* IoTHubServiceClient_GetVersionString(void);
```

## IoTHubServiceClient_GetVersionString
```c
const char* IoTHubServiceClient_GetVersionString(void)
```
**SRS_IOTHUBSERVICECLIENT_VERSION_12_001: [** IoTHubServiceClient_GetVersionString shall return a pointer to a constant string which indicates the version of IoTHubServiceClient API. **]**
