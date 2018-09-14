# Remote Monitoring solution accelerator sample

This sample illustrates how to connect to the Remote Monitoring solution accelerator to:

- Report device capabilities
- Send telemetry
- Respond to methods, including a long-running firmware update method

For more information about this sample, see:

- [Connect your device to the Remote Monitoring solution accelerator (Windows)](https://docs.microsoft.com/azure/iot-accelerators/iot-accelerators-connecting-devices)
- [Connect your device to the Remote Monitoring solution accelerator (Linux)](https://docs.microsoft.com/azure/iot-accelerators/iot-accelerators-connecting-devices-linux)

## Build and run in Windows

Edit the remote_monitoring.c to add your device connection string.

Follow the [instructions to build the SDK and samples](../../../doc/devbox_setup.md#build-the-c-sdk-in-windows) in Windows.

After you build the SDK, the remote monitoring client executable is located in the **azure-iot-sdk-c\cmake\samples\solutions\remote_monitoring\Release** folder.

## Build and run in Linux

Edit the remote_monitoring.c to add your device connection string.

Follow the [instructions to build the SDK and samples](../../../doc/devbox_setup.md#build-the-c-sdk-in-linux) in Linux.

After you build the SDK, the remote monitoring client executable is located in the **azure-iot-sdk-c/cmake/samples/solutions/remote_monitoring/** folder.