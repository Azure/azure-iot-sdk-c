# Digital Twin C SDK E2E Test Framework

## Overview

The files in this directory implement the device SDK portion of E2E testing of the C SDK for Digital Twin.  For example, there are tests to make sure that an invocation to a Digital Twin command, going through an IoTHub for real (NOT UT), are properly received on a simulated device running the device SDK.

This is complicated by the fact that the C SDK does *not* have a Digital Twin service SDK.  So a naive approach of `CreateProcess(dt_e2e.exe)` and having said process run the service and device SDK at the same time, checking internal state of each, will not work.

The e2e tests for Digital Twin use a different approach, splitting the service and device portions into distinct units of execution.  Service application code, written in TypeScript using the Digital Twin service SDK, will act as a test driver to an executable written in C simulating the device.

## How to run the tests
* Note: Currently these tests are only supported to run on Windows. Linux support will follow.

* Like any E2E test in the C SDK, the cmake flag `-Drun_e2e_tests=ON` must be configured for [device](./device) to even compile in the first place.

* The service application test is written in TypeScript.  It uses [Mocha](https://mochajs.org/) as its test framework and the Digital Twin and IoTHub service sdks, among other dependencies.  

* To run the e2e test framework:
  
  * cmake and build the project with -Drun_e2e_tests=ON.

  * Set your environment variable `IOTHUB_CONNECTION_STRING` to your IoTHub (**not** a device) connection string.

  * From your cmake binary directory, call 'ctest -C "debug" -R "iothubclient_dt_e2e"' to run just this test.
  
  * Note: if an error pops up stating that it was unable to find .js files, the transpile may have failed, check your npm installs.

## Service application and device interaction

How do the Node service application and the C simulated device interact?

* The [service](./service) directory contains code authored in TypeScript.  This code simulates a service application using Digital Twin SDK's interacting with a Digital Twin device.  The code:

  * Creates a device on the IoThub, using IoTHub (not Digital Twin specific) API's.  This device is prefixed with the name `digitaltwin-test-c-sdk` followed by a random string for uniqueness.

  * Creates a process built from the [device](/.device) directory, passing the connection string of the newly created device as the only command line argument.

  * Waits a few seconds for the test device to register.

  * Tests the device, sending various commands and property updates and checking the state.

  * On test completion, the test will:
  
    * Send a special command telling the test to gracefully terminate.

    * Delete the device on IoTHub.

    * Attempt to terminate the test's process, assuming there is some failure where it cannot gracefully exit.

* The [device](/.device) directory contains a simulated Digital Twin device.  It is written in C and is the component under test.

  * On test initiation, the device uses the passed in connection sting to register its test interfaces.  
  
  * The device will register three interfaces - one for command testing, one for telemetry, and one for properties.  
  
  * After  registration, the test takes **NO** action on its own.  Instead it waits for the service application to trigger it to perform actions.  The service, not the device, completely drives the test run.

  * Tests are initiated by either a command arriving - for command and telemetry testing - or by a command *and/or* a property update arriving for property tests.

  * Some tests make take an unknown amount of time to run.  For instance, verifying that a Digital Twin telemetry message was acknowledged may take a few seconds or more.  
  
    For this class of "need-to-poll" type test, the test command will return a `202` to indicate to the service when expected acknowledgements have not arrived.  The TypeScript code invoking the service SDK will re-query a set number of times until a `200` is returned, there is a failure, or the number of retries runs out.
