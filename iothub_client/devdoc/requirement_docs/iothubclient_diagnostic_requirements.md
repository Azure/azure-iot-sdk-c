#IoTHubClient Diagnostic Requirements

##Overview
The IoTHubClient_Diagnostic component is used to add predefined diagnostic properties to message for end to end diagnostic purpose

##Exposed API

```c
#define E2E_DIAG_SETTING_METHOD_VALUE  \
    E2E_DIAG_SETTING_UNKNOWN,          \
    E2E_DIAG_SETTING_USE_LOCAL,        \
    E2E_DIAG_SETTING_USE_REMOTE

DEFINE_ENUM(E2E_DIAG_SETTING_METHOD, E2E_DIAG_SETTING_METHOD_VALUE);

/** @brief diagnostic related setting */
typedef struct IOTHUB_DIAGNOSTIC_SETTING_DATA_TAG
{
    uint32_t diagSamplingPercentage;
    uint32_t currentMessageNumber;
    E2E_DIAG_SETTING_METHOD diagSettingMethod;
} IOTHUB_DIAGNOSTIC_SETTING_DATA;

extern int IoTHubClient_Diagnostic_AddIfNecessary(IOTHUB_DIAGNOSTIC_SETTING_DATA* diagSetting, IOTHUB_MESSAGE_HANDLE messageHandle);

extern int IoTHubClient_Diagnostic_UpdateFromTwin(IOTHUB_DIAGNOSTIC_SETTING_DATA* diagSetting, bool isPartialUpdate, const unsigned char* payLoad, STRING_HANDLE message);
```

## IoTHubClient_Diagnostic_AddIfNecessary 
```c
extern int IoTHubClient_Diagnostic_AddIfNecessary(IOTHUB_DIAGNOSTIC_SETTING_DATA* diagSetting, IOTHUB_MESSAGE_HANDLE messageHandle);
```

**SRS_IOTHUB_DIAGNOSTIC_13_001: [**IoTHubClient_Diagnostic_AddIfNecessary should return nonezero if diagSetting or messageHandle is NULL.**]**

**SRS_IOTHUB_DIAGNOSTIC_13_002: [**IoTHubClient_Diagnostic_AddIfNecessary should return nonezero if failing to add diagnostic property.**]** 

**SRS_IOTHUB_DIAGNOSTIC_13_003: [**If diagSamplingPercentage is equal to 0, message number should not be increased and no diagnostic properties added.**]**

**SRS_IOTHUB_DIAGNOSTIC_13_004: [**If IoTHubMessage_SetDiagnosticPropertyData finishes successfully it shall return IOTHUB_MESSAGE_OK.**]**

**SRS_IOTHUB_DIAGNOSTIC_13_005: [**If diagSamplingPercentage is between(0, 100), diagnostic properties should be added based on percentage.**]**

## IoTHubClient_Diagnostic_UpdateFromTwin
```c
extern int IoTHubClient_Diagnostic_UpdateFromTwin(IOTHUB_DIAGNOSTIC_SETTING_DATA* diagSetting, bool isPartialUpdate, const unsigned char* payLoad, STRING_HANDLE message)
```

**SRS_IOTHUB_DIAGNOSTIC_13_006: [**IoTHubClient_Diagnostic_UpdateFromTwin should return nonezero if arguments are NULL.**]**

**SRS_IOTHUB_DIAGNOSTIC_13_007: [**IoTHubClient_Diagnostic_UpdateFromTwin should return nonezero if payLoad is not a valid json string.**]**

**SRS_IOTHUB_DIAGNOSTIC_13_008: [**IoTHubClient_Diagnostic_UpdateFromTwin should return nonezero if device twin json doesn't contains a valid desired property.**]**

**SRS_IOTHUB_DIAGNOSTIC_13_009: [**IoTHubClient_Diagnostic_UpdateFromTwin should set diagSamplingPercentage = 0 when sampling rate in twin is null.**]**

**SRS_IOTHUB_DIAGNOSTIC_13_010: [**IoTHubClient_Diagnostic_UpdateFromTwin should set diagSamplingPercentage = 0 when cannot parse sampling rate from twin.**]**

**SRS_IOTHUB_DIAGNOSTIC_13_011: [**IoTHubClient_Diagnostic_UpdateFromTwin should set diagSamplingPercentage = 0 if sampling rate parsed from twin is not between [0,100].**]**

**SRS_IOTHUB_DIAGNOSTIC_13_012: [**IoTHubClient_Diagnostic_UpdateFromTwin should set diagSamplingPercentage correctly if sampling rate is valid.**]**

**SRS_IOTHUB_DIAGNOSTIC_13_013: [**IoTHubClient_Diagnostic_UpdateFromTwin should report diagnostic property not existed if there is no sampling rate in complete twin.**]**

**SRS_IOTHUB_DIAGNOSTIC_13_014: [**IoTHubClient_Diagnostic_UpdateFromTwin should return nonzero if STRING_sprintf failed.**]**
