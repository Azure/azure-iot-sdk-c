#IoTHubClient Diagnostic Requirements

##Overview
The IoTHubClient_Diagnostic component is used to add predefined diagnostic properties to message for end to end diagnostic purpose

##Exposed API

```c
typedef struct IOTHUB_DIAGNOSTIC_SETTING_DATA_TAG
{
    uint32_t diagSamplingPercentage;
    uint32_t currentMessageNumber;
} IOTHUB_DIAGNOSTIC_SETTING_DATA;
 
extern int IoTHubClient_Diagnostic_AddIfNecessary(IOTHUB_DIAGNOSTIC_SETTING_DATA* diagSetting, IOTHUB_MESSAGE_HANDLE messageHandle);

```

##IoTHubClient_Diagnostic_AddIfNecessary 
```c
extern int IoTHubClient_Diagnostic_AddIfNecessary(IOTHUB_DIAGNOSTIC_SETTING_DATA* diagSetting, IOTHUB_MESSAGE_HANDLE messageHandle);
```

**SRS_IOTHUB_DIAGNOSTIC_13_001: [**IoTHubClient_Diagnostic_AddIfNecessary should return nonezero if diagSetting or messageHandle is NULL.**]**

**SRS_IOTHUB_DIAGNOSTIC_13_002: [**IoTHubClient_Diagnostic_AddIfNecessary should return nonezero if failing to add diagnostic property.**]** 

**SRS_IOTHUB_DIAGNOSTIC_13_003: [**If diagSamplingPercentage is equal to 0, message number should not be increased and no diagnostic properties added.**]**

**SRS_IOTHUB_DIAGNOSTIC_13_004: [**If IoTHubMessage_SetDiagnosticPropertyData finishes successfully it shall return IOTHUB_MESSAGE_OK.**]**

**SRS_IOTHUB_DIAGNOSTIC_13_005: [**If diagSamplingPercentage is between(0, 100), diagnostic properties should be added based on percentage.**]**
