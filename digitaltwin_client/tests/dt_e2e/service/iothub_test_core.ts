// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// 
//  iothub_test_core.ts implements fundamental logic for the C SDK E2E test framework.
//
'use strict'

const { exec } = require('child_process');
var iothubDevice = require('azure-iothub').Device;
var iothubRegistry  = require('azure-iothub').Registry;
var uuid = require('uuid');
var assert = require('chai').assert;
var terminate = require('terminate');

// "Handle" to test executable.  Because this is global, means that there
// can only be one sub-process test executable per suite.
var test_exe = null

//
// executeTestProcess creates a worker process that hosts the C SDK test framework
// This also redirects stdout/stderr from the child process to the console
//
function executeTestProcess(test_process:string, cmd_args:string, resultCallback) {
    // Start the process
    var command_line:string = test_process + ` "` + cmd_args + `"`
    console.log(`Invoking exec(${command_line})`)
    test_exe = exec(command_line);
    
    test_exe.stdout.on('data', (data:string) => {
        console.log("APP STDOUT: " + data)
    })

    test_exe.stderr.on('data', (data:string) => {
        console.log("APP STDERR:" + data)
    })
    
    test_exe.on('close', (error:number) => {
        var result:number

        if (error) {
            console.warn(`process return non zero exit code error: ${error}`);
            result = 1;
        }
        else {
            console.log(`test process has cleanly exited`)
            result = 0;
        }

        return resultCallback(result);
    });
}

//
// terminateTestProcess will terminate (killproc) the test child process, if it is active.
//
function terminateTestProcessIfNecessary(done) {
    if (test_exe != null) {
        terminate(test_exe.pid, function(err, result) {
            if (err) {
                // Terminating the test process is a best effort action.  Its failure will not
                // cause an otherwise successful test run to be marked as failing.
                console.warn(`Unable to terminate test process ID=<${test_exe.pid}>`)
                done()
            }
            else {
                console.log(`Successfully terminated process`)
                done()
            }
        })
    }
}

//
// createTestDevice will create a new device on IoThub for these tests.  The name of the device
// is randomly determined, but will be prefixed with the specified testDeviceNamePrefix to make
// it easier for cleaning up test suites under development.  On device creation, the passed
// resultCallback function will be invoked.
//
function createTestDevice(hubConnectionString:string, testDeviceNamePrefix:string, resultCallback:any) {
    var registry = iothubRegistry.fromConnectionString(hubConnectionString)
    
    var testDeviceName:string = testDeviceNamePrefix + (Math.floor(Math.random() * 10000000000)).toString()

    var new_device = 
    {
        deviceId: testDeviceName,
        status: 'enabled',
        authentication: {
            symmetricKey: {
                primaryKey: new Buffer(uuid.v4()).toString('base64'),
                secondaryKey: new Buffer(uuid.v4()).toString('base64')
            }
        }
    }

    registry.create(new_device, resultCallback)
}

//
// Removes device of given name from IoTHub
// 
function deleteDevice(hubConnectionString:string, testDeviceName:string, resultCallback:any) {
    console.log(`About to delete device <${testDeviceName}>`)
    var registry = iothubRegistry.fromConnectionString(hubConnectionString)
    registry.delete(testDeviceName, resultCallback)
}

//
// getHubnameFromConnectionString retrieves the hostname from a given hub connection string
//
function getHubnameFromConnectionString(hubConnectionString:string) {
    // For given hub connection string, remove the HostName=<USER_SPECIFIED_VALUE>;Etc. so it's just the <sUSER_SPECIFIED_VALUE>
    return (hubConnectionString.replace("HostName=","")).replace(/;.*/,"")
}

//
// getConnectionStringFromDeviceInfo returns a connection string for a given device 
// based on the hub's connection string and the device's info structure.
//
function getConnectionStringFromDeviceInfo(hubConnectionString:string, deviceInfo) {
    // The deviceInfo doesn't contain the hub hostname, so need to parse it from initial connection string
    var hubHostName = getHubnameFromConnectionString(hubConnectionString)
    return ("HostName=" + hubHostName + ";DeviceId=" + deviceInfo.deviceId + ";SharedAccessKey=" + deviceInfo.authentication.symmetricKey.primaryKey)
}


module.exports = {
    getConnectionStringFromDeviceInfo : getConnectionStringFromDeviceInfo,
    getHubnameFromConnectionString : getHubnameFromConnectionString,
    createTestDevice : createTestDevice,
    executeTestProcess : executeTestProcess,
    deleteDevice : deleteDevice,
    terminateTestProcessIfNecessary : terminateTestProcessIfNecessary
}
