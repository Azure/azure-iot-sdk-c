# CocoaPods for Microsoft Azure IoT

The [Microsoft Azure IoT C SDK](https://github.com/azure/azure-iot-sdk-c) is available as five Objective-C [CocoaPods](https://cocoapods.org/):

* [AzureIoTUtility](https://cocoapods.org/?q=AzureIoTUtility) contains the [Azure IoT C Shared Utility library](https://github.com/Azure/azure-c-shared-utility)
* [AzureIoTuAmqp](https://cocoapods.org/?q=AzureIoTuAmqp) contains the [Azure IoT AMQP library](https://github.com/Azure/azure-uamqp-c)
* [AzureIoTuMqtt](https://cocoapods.org/?q=AzureIoTuMqtt) contains the [Azure IoT MQTT library](https://github.com/Azure/azure-umqtt-c)
* [AzureIoTHubClient](https://cocoapods.org/?q=AzureIoTHubClient) contains the [Azure IoT Hub Client](https://github.com/azure/azure-iot-sdk-c)
* [AzureIoTHubServiceClient](https://cocoapods.org/?q=AzureIoTHubServiceClient) contains the [Azure IoT Hub Service Client](https://github.com/azure/azure-iot-sdk-c)

## Samples

Samples in Swift for iOS are [here](https://github.com/Azure-Samples/azure-iot-samples-ios.git).

## Using Azure IoT CocoaPods with Objective-C

Using Objective-C libraries within another Objective-C app or library requires setting up header search paths just like typical C libraries require

The header search path values you'll need to set for the Azure IoT CocoaPods are:

* `${PODS_ROOT}/AzureIoTHubClient/inc/`
* `${PODS_ROOT}/AzureIoTHubServiceClient/inc/`
* `${PODS_ROOT}/AzureIoTUtility/inc/`
* `${PODS_ROOT}/AzureIoTuMqtt/inc/`
* `${PODS_ROOT}/AzureIoTuAmqp/inc/`

## Using Azure IoT CocoaPods with Swift

Swift apps and libraries can use Azure IoT CocoaPods with standard Swift imports:

* `import AzureIoTHubClient`
* `import AzureIoTHubServiceClient`
* `import AzureIoTuAmqp`
* `import AzureIoTuMqtt`
* `import AzureIoTUtility`
