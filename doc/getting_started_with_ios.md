# Getting start with IoTHub C SDK on iOS

The SDK produces Ios CocoaPods to ease the writing of end user application on Ios devices.

## Prerequisites

You will need these prerequisites to run the samples:

* The latest version of XCode

* The latest version of the iOS SDK

* [The latest version of CocoaPods](https://guides.cocoapods.org/using/index.html) and familiarity with their basic usage. Some more detail about the Azure IoT CocoaPods may be found [here](./sdk_cocoapods.md).

* An IoT Hub and a connection string for a client device

## Running the iothub samples

### Clone the Azure IoT C SDK repo

Change to a location where you would like your samples, and run

```Shell
git clone https://github.com/Azure-Samples/azure-iot-samples-ios.git
```

### Navigate to the iOS sample directory

Change your current directory to the iOS sample directory they you would like to run

for device client:

```Shell
cd azure-iot-ios-samples/quickstart/sample-device
```

for service client:

```Shell
cd azure-iot-ios-samples/quickstart/sample-service
```

### Install the CocoaPods

Make sure that XCode does not already have the sample project open. If
it does, the CocoaPods may not install properly

Run this command:

```Shell
pod install
```

This will cause CocoaPods to read the `Podfile` and install the pods accordingly

### Open the XCode workspace

Double-click the `MQTT Client Sample.xcworkspace` workspace file for the device sample or the `AzureIoTServiceSample.xcworkspace` workspace file for the service sample.

### Modify Device Samples Project

1. Replace the `private let connectionString = "[device connection string]"` string with the connection string for your provisioned device.

2. Start the project (command-R) to see the sdk run.
