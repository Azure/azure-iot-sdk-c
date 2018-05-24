# Building iOS samples for Azure IoT with CocoaPods

#### Prerequisites
 You will need these prerequisites to run the samples:
* The latest version of XCode
* The latest version of the iOS SDK
* [The latest version of CocoaPods](https://guides.cocoapods.org/using/index.html) and 
familiarity with their basic usage. Some more detail about the Azure IoT CocoaPods
may be found [here](./CocoaPods.md).
* An IoT Hub and a connection string for a client device.

#### 1. Clone the Azure IoT C SDK repo

Change to a location where you would like your samples, and run

`git clone https://github.com/Azure/azure-iot-sdk-c.git`

It is not necessary to do a recursive clone to build the iOS samples.

#### 2. Navigate to the iOS sample directory

Change your current directory to the iOS sample directory.

`cd azure-iot-sdk-c/iothub_client/samples/ios/AzureIoTSample`

**Note:** The sample does not need to be within the SDK tree, so you may copy it to a 
convenient location instead of building it within the tree.

#### 3. Install the CocoaPods

Make sure that XCode does not already have the sample project open. If
it does, the CocoaPods may not install properly.

Run this command:

`pod install`

This will cause CocoaPods to read the `Podfile` and install the pods accordingly.

#### 4. Copy an Azure IoT sample into your iOS project

The `azure-iot-sdk-c/iothub_client/samples` directory some Azure IoT samples that
are appropriate for working with the iOS sample application:

* **iothub_ll_c2d_sample** (cloud to device) This sample demonstrates sending messages
from the cloud to the device. Device Explorer can be used to send the messages.
* **iothub_ll_telemetry_sample** The telemetry sample demonstrates sending messages
from the device to the cloud. See 
[here](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-messages-d2c) for details.
* **iothub_client_sample_device_twin** This sample demonstrates how to use device 
twins in IoT Hub.  See [here](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-device-twins) 
for details.

Each sample can use one of the five communication protocols supported: HTTP, MQTT, 
MQTT over Websockets, AMQP, and AMQP over Websockets.

Choose one of these samples and copy its `.c` file into your AzureIoTSample project directory
(it should be in the same directory with `ios-sample.h`).

#### 5. Open the XCode workspace

Double-click the `AzureIoTSampleWorkspace.xcworkspace` workspace file (**not** the project file) to
open XCode and select your build target device (iPhone 7 simulator works well).

Make sure you open the workspace, and not the similarly-named (without the `WS` suffix) project.

#### 4. Modify your sample file

1. Select the AzureIoTSample project, choose "Add files...", and add the sample `.c` file that
that you chose in Step 4.
2. Open the `.c` sample file that you just added.
3. Add the line `#include "ios-sample.h"` below all the other include files.
4. Near the top of the file, uncomment a single protocol that you want to use:
    * SAMPLE_HTTP
    * SAMPLE_MQTT
    * SAMPLE_MQTT_OVER_WEBSOCKETS
    * SAMPLE_AMQP
    * SAMPLE_AMQP_OVER_WEBSOCKETS
5. Replace the `<insert connections string here>` string with
the connection string for your provisioned device.

**Note:** The file you just modified is the only file in the iOS sample that needs any modification
for the sample to work. 

#### 5. Run the app in the simulator

Start the project (command-R). The sample is designed for landscape viewing, so you may need to
rotate the simulator. 

The `iothub_ll_telemetry_sample` will send messages to the cloud and report its responses, as will
iothub_client_sample_device_twin.
The `iothub_ll_c2d_sample` will wait for messages from the cloud and display them
when they arrive. Device Explorer can send such messages.


