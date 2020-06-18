IOTHUB_DEVICE_CLIENT_HANDLE InitializeIoTHubDeviceHandleForPnP(
    const char* modelId, 
    bool enableTracing,
    IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback,
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback
    )
{
    IOTHUB_DEVICE_CLIENT_HANDLE deviceHandle;

    (void)IoTHub_Init();

    deviceHandle = IoTHubDeviceClient_CreateFromConnectionString(connectionString, MQTT_Protocol());
    if (device_handle == NULL)
    {
        (void)printf("Failure creating Iothub device.  Hint: Check you connection string.\r\n");
    }
    else
    {
        // Sets the callback function that processes incoming device methods from the hub.
        (void)IoTHubDeviceClient_SetDeviceMethodCallback(device_handle, device_method_callback, NULL);

        // Sets the callback function that processes incoming twin properties from the hub.  This will
        // also automatically retrieve the full twin for the application.
        (void)IoTHubDeviceClient_SetDeviceTwinCallback(iotHubClientHandle, deviceTwinCallback, NULL);

        // Sets the name of ModelId for this PnP device.
        (void)IoTHubDeviceClient_SetOption(device_handle, OPTION_MODEL_ID, modelId);

        // Sets the level of verbosity of the logging
        if (enableTracing == true)
        {
            (void)IoTHubDeviceClient_SetOption(device_handle, OPTION_LOG_TRACE, &enableTracing);
        }

        // Enabling auto url encode will have the underlying SDK perform URL encoding operations automatically for the application.
        bool urlEncodeOn = true;
        (void)IoTHubDeviceClient_SetOption(device_handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        // Setting the Trusted Certificate.  This is only necessary on system with without
        // built in certificate stores.
        (void)IoTHubDeviceClient_SetOption(device_handle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES
    }

    return deviceHandle;
}

