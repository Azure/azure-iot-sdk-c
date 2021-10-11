# Azure IoT C SDK Samples

The Azure IoT C SDK contains components that can operate independently that are separated into directories at the top level of the repo.

The samples for these components live with them in the directory tree, not in a global **samples** directory.

The samples available throughout the tree are:

* IoTHub Device Client SDK samples are [here](../iothub_client/samples).  *If you're just trying to build an IoT Device, this is going to be the best place to start.*
* IoTHub Service SDK samples are [here](../iothub_service_client/samples)
* Serializer samples are  [here](../serializer/samples).  *Please read and understand the limitations and alternatives to the serializer described [here](../serializer/readme.md) before using the serializer.*
* Provisioning Client SDK samples are [here](../provisioning_client/samples)
* Provisioning Service SDK samples are [here](../provisioning_service_client/samples)

You can find samples for end-to-end samples scenarios [here](solutions).