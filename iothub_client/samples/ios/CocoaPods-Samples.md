# Building iOS samples for Azure IoT with CocoaPods

#### Prerequisites
 You will need these prerequisites to run the samples:
* The latest version of XCode
* The latest version of the iOS SDK
* [The latest version of CocoaPods](https://guides.cocoapods.org/using/index.html) and 
familiarity with their basic usage. Some more detail about the Azure IoT CocoaPods
may be found [here](./CocoaPods.md).
* An IoT Hub and a connection string for a client device.
* [Azure IoT Explorer](https://github.com/Azure/azure-iot-explorer) recommended.

#### 1. Clone the Azure IoT iOS Sample

Change to a location where you would like your samples, and run

`git clone https://github.com/Azure-Samples/azure-iot-samples-ios.git`


#### 2. Navigate to the device sample directory

Change your current directory to the iOS sample directory.

`cd azure-iot-samples-ios/quickstart/sample-device/`

#### 3. Update the Podfile

Note: Make sure that XCode does not already have the sample project open. If
it does, the CocoaPods may not install properly.

Edit the Podfile in a text editor and ensure that the pods are specified with these versions or later:

`'AzureIoTHubClient', '=1.3.8a'`
`'AzureIoTUtility', '=1.3.8a'`
`'AzureIoTuMqtt', '=1.3.8a'`
`'AzureIoTuAmqp', '=1.3.8a'`

#### 3. Install the CocoaPods

Make sure that XCode does not already have the sample project open. If
it does, the CocoaPods may not install properly.

Run this command:

`pod install`

This will cause CocoaPods to read the `Podfile` and install the pods accordingly.

#### 5. Open the XCode workspace

Double-click the `MQTT Client Sample.xcworkspace` workspace file (**not** the project file) to
open XCode and select your build target device (iPhone 7 simulator works well).

Make sure you open the workspace, and not the similarly-named (without the `WS` suffix) project.

#### 4. Modify your sample file

1. Select the MQTT Cleint Sample project, open the MQTT Client Sample folder, and open the ViewController.swift
2. Add your iot device Connection String to the `private let connectionString` by replacing the empty quotes with your connection string.
3. Below the connectionString variable, select a single protocol that you want to use, and assign it to the `iotProtocol` variable:
    * HTTP_Protocol
    * MQTT_Protocol
    * AMQP_Protocol
    Note: HTTP_Protocol does work as well. 

#### 5. Run the app in the simulator

Start the project (command-R). 

