# Run a simple device Twin C sample on Windows

## Table of Contents

-   [Introduction](#Introduction)
-   [Step 1: Prerequisites](#Step-1-Prerequisites)
-   [Step 2: Build and Run the Sample](#Step-2-Build)

<a name="Introduction"></a>

## Introduction

**About this document**

This document describes how to build and run sample applications on the Windows platform. This multi-step process includes:
-   Preparing your development environment
-   Creating and configuring and instance of Azure IoT Hub
-   Build and deploy Azure IoT SDK on device

<a name="Step-1-Prerequisites"></a>

## Step 1: Prerequisites

You should have the following items ready before beginning the process:

-   [Prepare your development environment][devbox-setup]
-   [Setup your IoT hub][lnk-setup-iot-hub]
-   [Provision your device and get its credentials][lnk-manage-iot-hub]

<a name="Step-2-Build"></a>

## Step 2: Build and Run the sample

1.  Start a new instance of Visual Studio 2015. Open the **azure_iot_sdks.sln** solution in the **cmake_win32** folder in your home directory (usually C:\\\\users\\username\\).

2.  In Visual Studio, in **Solution Explorer**, navigate to and open the following file:

    IoTHub_Samples\iothub_client_device_twin_and_methods_sample\Source Files\iothub_client_device_twin_and_methods_sample.c
    

3.  Locate the following code in the file:

      ```
      static const char* connectionString = "[device connection string]";
      ```

4.  Replace "[device connection string]" with the device connection string you noted [earlier](#Step-1-Prerequisites) and save the changes:

       ```
       static const char* connectionString = "HostName=..."
       ```
       
5.  In **Solution Explorer**, right-click the project IoTHub_Samples\iothub_client_device_twin_and_methods_sample, click **Debug**, and then click **Start new instance** to build and run the sample. 
    
6.  As the client is running it will receive the current complete Twin json content, as well as send an update to the reported properties.


[lnk-setup-iot-hub]: ../../../doc/setup_iothub.md
[lnk-manage-iot-hub]: ../../../doc/manage_iot_hub.md
[devbox-setup]: ../../../doc/devbox_setup.md
