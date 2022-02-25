---
name: Bug Report
about: Create a bug report to help us improve
title: ''
labels: 
assignees: ''

---

------------------------------- delete below -------------------------------

INSTRUCTIONS
==========

Please follow the instructions and template below to save us time requesting additional information from you.

1. Search existing issues to avoid creating duplicates.

2. If possible test using the latest release to make sure your issues has not already been fixed: 
https://github.com/Azure/azure-iot-sdk-c/releases/latest 

3. Do not share information from your Azure subscription here (connection strings, service names (IoT Hub name, Device Provisioning Service scope ID), etc...). If you need to share any of this information, you can create a ticket and get assistance from the Microsoft Support.

How to Submit an Azure Support Ticket: https://docs.microsoft.com/azure/azure-supportability/how-to-create-azure-support-request


4. Include enough information for us to address the bug:

   -  A detailed description.
   -  A Minimal Complete Reproducible Example (https://stackoverflow.com/help/mcve). This is code we can cut and paste into a readily available sample and run, or a link to a project you've written that we can compile to reproduce the bug. 
   -  Console logs. If you are unsure how to enable logging, refer to this document: https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/Iothub_sdk_options.md

5. Delete these instructions before submitting the bug.



Below is a hypothetical bug report. We recommend you use it as a template and replace the information below each header with your own. 

------------------------------- delete above -------------------------------


**Development Machine, OS, Compiler (and Other Relevant Toolchain Info)**

Raspberry Pi, Raspbian Stretch Lite (Release 2018-11-13)
Cross Compiled on Ubuntu 18.04 using GCC 6.3.0

**SDK Version (Please Give Commit SHA if Manually Compiling)**

Release 2019-01-31

**Protocol**

MQTT

**Describe the Bug**

If MQTT is unable to establish a connection, it will keep trying and once it succeeds queued messages will be sent to the Cloud. However, if for some reason we can't get past the initial connection phase, then SDK does not respect message timeouts. 

**[MCVE](https://stackoverflow.com/help/mcve)**

```
#include "iothub.h"

int main(void)
{
    if (lightbulb == ON) {
        iothub_say_hello();
        return 0;
    } else {
        iothub_say_goodbye();
        return 1;
    }
}
```

**Console Logs**

Sending message 1 to IoTHub
-> 15:07:42 PUBLISH | IS_DUP: false | RETAIN: 0 | QOS: DELIVER_AT_LEAST_ONCE | TOPIC_NAME: devices/tracingDevice/messages/events/property_key=property_value | PACKET_ID: 93 | PAYLOAD_LEN: 12
<- 15:07:42 PUBACK | PACKET_ID: 92
Confirmation callback received for message 86 with result IOTHUB_CLIENT_CONFIRMATION_OK
