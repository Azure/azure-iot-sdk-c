---
name: Bug Report
about: Create a bug report to help us improve
title: ''
labels: bug
assignees: ''

---

------------------------------- delete below -------------------------------

INSTRUCTIONS
==========

If the below instructions are not followed, we may be forced to close your issue.

1. Search existing issues to avoid creating duplicates.

2. If possible test using the latest release to make sure your issues has not already been fixed: 
https://github.com/Azure/azure-iot-sdk-c/releases/latest 

3. Do not share information from your Azure subscription here (connection strings, service names (IoT Hub name, Device Provisioning Service scope ID), etc...). Please submit a ticket and get assistance from the Microsoft support team using the following link: 
https://docs.microsoft.com/en-us/azure/azure-supportability/how-to-create-azure-support-request

4. Include enough information for us to address the bug, including a description of the bug, a Minimal Complete Reproducible Example that demonstrates the bug, and any relevant logs related to the bug.  

For the MCVE, we recommend you provide us with a code snippet we can cut and paste into a readily available sample, or a link to a code sample that we can compile to reproduce the bug. 

If you are unsure how to enable logging, refer to this document: https://github.com/Azure/azure-iot-sdk-c/blob/master/doc/Iothub_sdk_options.md

5. Delete these instructions before submitting the bug.

------------------------------- delete above -------------------------------


**Device, OS (and Other Relevant Toolchain Info)**
Raspberry Pi, Raspbian Stretch Lite (Release 2018-11-13)

**SDK Version (Please Give Commit SHA if Manually Compiling)**
Release 2019-01-31

**Protocol**
MQTT

**Describe the Bug**
If MQTT is unable to establish a connection, it will keep trying and once it succeeds queued messages will be sent to the Cloud. However, if for some reason we can't get past the initial connection phase, then SDK does not respect message timeouts. 

**(MCVE)[https://stackoverflow.com/help/mcve]**
```
#include "iothub.h"

int main(void)
{
    return 0;
}
```

**Console Logs **
Sending message 87 to IoTHub
-> 15:07:42 PUBLISH | IS_DUP: false | RETAIN: 0 | QOS: DELIVER_AT_LEAST_ONCE | TOPIC_NAME: devices/tracingDevice/messages/events/property_key=property_value | PACKET_ID: 93 | PAYLOAD_LEN: 12
<- 15:07:42 PUBACK | PACKET_ID: 92
Confirmation callback received for message 86 with result IOTHUB_CLIENT_CONFIRMATION_OK
