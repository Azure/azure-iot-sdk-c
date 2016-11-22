# Microsoft Azure IoT serializer library for C

This folder contains the following 
* A helper library for serializing and deserializing data
* Samples showing how to use the serializer library

## Features
* Format the data you want to send to the Cloud and deserialize data received from the Cloud
* Simply declare a "model" for the device using Macros
* Easily implement Azure IoT Device Twins and Methods features

## Using the seralizer library for C

The seralizer library for C is used with the Azure IoT device SDK for which you will find detailed instructions on how to use on Linux, mbed, Windows and other platforms [here][device-sdk].

## Samples

The repository contains a set of simple samples that will help you get started.
You can find a list of these samples with instructions on how to run them [here][samples]. 


[devbox_setup]: ../doc/devbox_setup.md
[samples]: ./samples/
[device-sdk]: ../iothub_client/