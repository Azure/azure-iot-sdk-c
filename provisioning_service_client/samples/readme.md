# Samples for the Azure IoT Device Provisioning Service Client

## Overview 

This folder contains simple samples showing how to use the features of the Microsoft Azure IoT Hub Device Provisioning Service from a back-end application running C code, using the Azure IoT C SDK.

## Prerequisites

In order to run the samples, you will first need to do the following:
* [Set up your IoT Hub][setup-iot-hub]
* [Set up your Device Provisioning Service][setup-provisioning-service]

## How to compile and run the samples

[This document][devbox-setup] describes in detail how to prepare your development environment on Linux, Windows, and other platforms. When compiling, use the flag `-Duse_prov_client=ON` to enable the Provisioning Service Client.

## List of Samples

* [Individual Enrollment](prov_sc_individual_enrollment_sample) - Create, update, get and delete Individual Enrollments on the Provisioning Service.
* [Enrollment Group](prov_sc_enrollment_group_sample) - Create, update, get and delete Enrollment Groups on the Provisioning Service.
* [Bulk Enrollment](prov_sc_bulk_operation_sample) - Run bulk enrollment operations for Individual Enrollments on the Provisioning Service.
* [Query](prov_sc_query_sample) - Run a query operation on the Provisioning Service

[setup-iot-hub]: https://aka.ms/howtocreateazureiothub
[setup-provisioning-service]: https://docs.microsoft.com/azure/iot-dps/quick-setup-auto-provision
[devbox-setup]: ../../doc/devbox_setup.md