# CocoaPods for Microsoft Azure IoT

The [Microsoft Azure IoT C SDK](https://github.com/azure/azure-iot-sdk-c) is 
available as four Objective-C [CocoaPods](https://cocoapods.org/):
* [AzureIoTUtility](https://cocoapods.org/?q=AzureIoTUtility) contains the [Azure IoT C Shared Utility library](https://github.com/Azure/azure-c-shared-utility)
* [AzureIoTuAmqp](https://cocoapods.org/?q=AzureIoTuAmqp) contains the [Azure IoT AMQP library](https://github.com/Azure/azure-uamqp-c)
* [AzureIoTuMqtt](https://cocoapods.org/?q=AzureIoTuMqtt) contains the [Azure IoT MQTT library](https://github.com/Azure/azure-umqtt-c)
* [AzureIoTHubClient](https://cocoapods.org/?q=AzureIoTHubClient) contains the [Azure IoT Hub Client](https://github.com/azure/azure-iot-sdk-c)

Most applications will only need to specify the [AzureIoTHubClient](https://cocoapods.org/?q=AzureIoTHubClient)
in their Podfile, and it will bring in the others as dependencies.

#### Using Azure IoT CocoaPods with Swift

Swift apps and libraries can use Azure IoT CocoaPods with standard Swift imports:</br>
`import AzureIoTHubClient`</br>
`import AzureIoTuAmqp`</br>
`import AzureIoTuMqtt`</br>
`import AzureIoTUtility`</br>
