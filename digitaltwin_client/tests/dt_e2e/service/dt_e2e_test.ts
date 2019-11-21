// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// These TypeScript tests are the test driver for E2E tests, acting as service application
// calling into an EXE hosting the C DigitalTwin device SDK.
// See ../readme.md for a full architectural review of how it works.
'use strict'

var iothubTestCore = require ('./iothub_test_core')
var assert = require('chai').assert;

import {
    IotHubGatewayServiceAPIs20190701Preview as DigitalTwinServiceClient,
    IotHubGatewayServiceAPIs20190701PreviewModels as Models
} from './digitaltwin_service_sdk/iotHubGatewayServiceAPIs20190701Preview';

import { IoTHubTokenCredentials } from './digitaltwin_service_sdk/iothub_token_credentials'

// testDeviceInfo contains the settings for our test device on the IoTHub.
var testDeviceInfo:any = null
// Time, in ms, for test framework to sleep after the process is created and before continuing next tests.
const sleepAfterProcessCreation = 5000

// InterfaceIds for interfaces implemented automatically by SDK
const sdkInformationInterfaceId = "urn:azureiot:Client:SDKInformation:1"
const modelDiscoveryInterfaceId = "urn:azureiot:ModelDiscovery:ModelInformation:1"

//
// createTestDeviceAndTestProcess creates a device for test scenario and creates the test process that hosts the C SDK
//
function createTestDeviceAndTestProcess(done) {
    // First, we create a new test device on IoTHub.  This will be destroyed on test tear down.
    iothubTestCore.createTestDevice(testHubConnectionString, "digitaltwin-test-c-sdk", function(err, newDeviceInfo) {
        if (err) {
            console.log(`createTestDevice fails, error=<${err}>`)
            done(err)
        }
        else {
            var testDeviceConnectionString:string = iothubTestCore.getConnectionStringFromDeviceInfo(testHubConnectionString, newDeviceInfo)
            console.log(`Successfully new device with connectionString=<${testDeviceConnectionString}>`)

            // Store information about this device for later
            testDeviceInfo = newDeviceInfo

            // Creates the test process, providing connection string of the test created device it should associate with
            console.log(`Invoking test executable ${testDeviceConnectionString}`)
            iothubTestCore.executeTestProcess(argv.testexe, testDeviceConnectionString, function processCompleteCallback(result:number) {
                console.log("Test process has completed execution")
            })

            // Now we need to wait until the test EXE is registered.
            console.log(`Going to sleep for ${sleepAfterProcessCreation}`)
            setTimeout(function() { done() }, sleepAfterProcessCreation)
        }
    })
}

//
// deleteDeviceAndEndProcess deletes the device and forces the child test process, if still running, to terminate.
//
function deleteDeviceAndEndProcess(done) {
    if (testDeviceInfo != null) {
        // First we signal to the running process to terminate gracefully, with a special DigitalTwin command for the purpose.
        const client:DigitalTwinServiceClient = GetDigitalTwinServiceClient()
    
        console.log(`Sending command to test EXE to gracefully shutdown`)
        client.digitalTwin.invokeInterfaceCommand(testDeviceInfo.deviceId, testCommandInterfaceName, "TerminateTestRun", "",  function processCallback() {
            // Don't bother checking error code here.  This graceful shutdown is a best effort, so as to avoid having to kill the process.

            iothubTestCore.deleteDevice(testHubConnectionString, testDeviceInfo.deviceId, function() {
                // Always attempt to terminate the test process, regardless of whether device deletion succeeded or failed
                iothubTestCore.terminateTestProcessIfNecessary(done)
            })
        })
    }
    else {
        done()
    }
}

//
// GetDigitalTwinServiceClient returns a DigitalTwinServiceClient, configured with
// appropriate hub credentials and apiVersion
//
function GetDigitalTwinServiceClient():DigitalTwinServiceClient {
    const creds = new IoTHubTokenCredentials(testHubConnectionString);
   
    const opts: Models.IotHubGatewayServiceAPIs20190701PreviewOptions = {
      // apiVersion: ,
      baseUri: "https://" + iothubTestCore.getHubnameFromConnectionString(testHubConnectionString)
    }
    
    const client = new DigitalTwinServiceClient("2019-07-01-preview", creds, opts)
    return client
}

//
// getTestHubConnectionString returns hub's connection string, from either command line or environment
//
function getTestHubConnectionString():string {
    if (argv.connectionString != null) {
        return argv.connectionString
    }
    else if (process.env.IOTHUB_CONNECTION_STRING != null) {
        return process.env.IOTHUB_CONNECTION_STRING
    }
    else {
        throw "Hub connection string must be configured in the command line or environment variable IOTHUB_CONNECTION_STRING"
    }
}


//
// CommandTestData stores the expected test data for a particular test run.  ALL tests - telemetry, properties, and commands - 
// are initiated by a command that this test framework invokes on the test executable.
//
class CommandTestData {
    methodName:string;
    payload:JSON;
    expectedResponseStatusCode:number;
    expectedResponsePayload:string;
    interfaceId:string;
    interfaceName:string

    constructor(methodName:string, payload:string, expectedResponseStatusCode:number, expectedResponsePayload:string, interfaceId:string, interfaceName:string) {
        this.methodName = methodName
        this.payload = JSON.parse(payload)
        this.expectedResponseStatusCode = expectedResponseStatusCode
        this.expectedResponsePayload = JSON.parse(expectedResponsePayload)
        this.interfaceId = interfaceId
        this.interfaceName = interfaceName
    }
}

//
// CommandTestData_TestCommands is used for testing DigitalTwin commands
//
const testCommandInterfaceId = "urn:azureiot:testinterfaces:cdevicesdk:commands:1"
const testCommandInterfaceName = "testCommands"
class CommandTestData_TestCommands extends CommandTestData {

    constructor(methodName:string, payload:string, expectedResponseStatusCode:number, expectedResponsePayload:string) {
        super(methodName, payload, expectedResponseStatusCode, expectedResponsePayload, testCommandInterfaceId, testCommandInterfaceName)
    }
}

//
// CommandTestData_TestTelemetry is used for testing DigitalTwin telemetry
//
const testTelemetryInterfaceID = "urn:azureiot:testinterfaces:cdevicesdk:telemetry:1"
const testTelemetryInterfaceName = "testTelemetry"
class CommandTestData_TestTelemetry extends CommandTestData {
    constructor(methodName:string, payload:string, expectedResponseStatusCode:number, expectedResponsePayload:string) {
        super(methodName, payload, expectedResponseStatusCode, expectedResponsePayload, testTelemetryInterfaceID, testTelemetryInterfaceName)
    }
}

//
// CommandTestData_PropertyTelemetry i sused for testing DigitalTwin properties
//
const testPropertyInterfaceId = "urn:azureiot:testinterfaces:cdevicesdk:properties:1"
const testPropertyInterfaceName = "testProperties"
class CommandTestData_PropertyTelemetry extends CommandTestData {
    constructor(methodName:string, payload:string, expectedResponseStatusCode:number, expectedResponsePayload:string) {
        super(methodName, payload, expectedResponseStatusCode, expectedResponsePayload, testPropertyInterfaceId, testPropertyInterfaceName)
    }
}

//  maxPollingAttempts specifies the number of times that a command will re-invoke itself when the device returns a 202.
const maxPollingAttempts = 5
// waitBetweenPollingAttempts specifies the time, in ms, between commands when we are in polling mode
const waitBetweenPollingAttempts = 2000

//
// runTestCommand invokes the test command on the test exe and verifies the results from the command are as expected.
// Note that some operations - such as updating properties or sending telemetry messages - are inherently asynchronous and 
// cannot finish in the time it takes the command to respond.  For these cases, runTestCommand is only the first
// stage in the test process and subsequent tests will need to verify the next stages.
//
function runTestCommand(testData:CommandTestData, numberInvocationAttempts:number, done) {
    const client:DigitalTwinServiceClient = GetDigitalTwinServiceClient()
    
    client.digitalTwin.invokeInterfaceCommand(testDeviceInfo.deviceId, testData.interfaceName, testData.methodName, testData.payload, 
    function processCallback(error, responsePayload, initialRequest, response) {
        if (error) {
            console.error("Invoking command failed, error = " + error)
            done(error)
        }
        else {
            // The HTTP status *must* be 200 for a successful e2e command.  This is NOT the response of the device itself
            //response.parsedHeaders.xMsCommandStatuscode
            assert.equal(200, response.status)

            console.log(`Received response from device method.  response code from device=<${response.parsedHeaders.xMsCommandStatuscode}>, responseBody=<${responsePayload}>`)

            // Some tests return a 202 to indicate some log-running operation on the device is active and not yet 
            // complete.  We'll kick into retry logic here, up to a max.
            if (response.parsedHeaders.xMsCommandStatuscode == 202) {
                if (numberInvocationAttempts == maxPollingAttempts) {
                    console.error("Test has exhausted maximum number of times it can retry.")
                    done(new Error('Maximum retry attempts have been tried'))
                }
                else {
                    console.log("Test executable returned a 202, so will attempt to query again")   
                    setTimeout(() => runTestCommand(testData, ++numberInvocationAttempts, done), waitBetweenPollingAttempts)
                }
            }
            else {
                // There is no polling, so we can proceed to check that the responses are as expected
                assert.equal(response.parsedHeaders.xMsCommandStatuscode, testData.expectedResponseStatusCode, "Status codes do not match")
                assert.equal(JSON.stringify(responsePayload), JSON.stringify(testData.expectedResponsePayload), "Payloads do not match")
                done(null, responsePayload, response.status)
            }            
        }
    })
}

//
// verifyPropertySet makes sure that the given property on given interface is set to the expected value
//
function verifyPropertySet(done, interfaceToCheck:string, propertyNameToCheck:string, expectedValue:string, numberInvocationAttempts:number) {
    const client:DigitalTwinServiceClient = GetDigitalTwinServiceClient()
  
    client.digitalTwin.getAllInterfaces(testDeviceInfo.deviceId, function(error, interfaceInfo) {
        if (error) {
            // An error is not immediately fatal, as there maybe delays between test querying and the device/IoTHub having time 
            // to store the request.  Use a simple polling mechanism.
            if (numberInvocationAttempts == maxPollingAttempts) {
                console.error("Test has exhausted maximum number of times it can retry.")
                done(new Error('Maximum retry attempts have been tried'))
            }
            else {
                console.log(`Interface ${interfaceToCheck} got expected value for property ${propertyNameToCheck} is not present yet, so retrying`)
                setTimeout(() => verifyPropertySet(done, interfaceToCheck, propertyNameToCheck, expectedValue, ++numberInvocationAttempts), waitBetweenPollingAttempts)
            }
        }
        else {
            if ((interfaceInfo.interfaces[interfaceToCheck] != null) && 
                (interfaceInfo.interfaces[interfaceToCheck].properties[propertyNameToCheck] != null) &&
                (interfaceInfo.interfaces[interfaceToCheck].properties[propertyNameToCheck].reported.value == expectedValue)) {
                console.log(`Interface ${interfaceToCheck} got expected value for property ${propertyNameToCheck}`)
                done()
            }
            else if (numberInvocationAttempts == maxPollingAttempts) {
                console.error("Test has exhausted maximum number of times it can retry.")
                done(new Error('Maximum retry attempts have been tried'))
            }
            else {
                console.log(`Interface ${interfaceToCheck} got expected value for property ${propertyNameToCheck} is not present yet, so retrying`)
                setTimeout(() => verifyPropertySet(done, interfaceToCheck, propertyNameToCheck, expectedValue, ++numberInvocationAttempts), waitBetweenPollingAttempts)
            }
        }
    })
}

//
// updateProperties updates DigitalTwin property remotely
//
function updateProperties(done) {
    const client:DigitalTwinServiceClient = GetDigitalTwinServiceClient()

    var interfacesPatchInfo:Models.DigitalTwinInterfacesPatch  = {
        interfaces: {
            "testProperties": {
                properties: {
                    ProcessUpdatedProperty1: {
                        desired: {
                            value: "ValueOfPropertyBeingSet_1"
                        }
                    }
                }   
            }
        }
    }

    client.digitalTwin.updateMultipleInterfaces(testDeviceInfo.deviceId, interfacesPatchInfo, function processUpdate(error, result) {
        if (error) {
            console.error("ERROR:" + error)
            done(error)
        }
        else {
            console.log("Success: " + result)
            done()
        }
    })
}

//
// verifySdkInfoPropertiesSet makes sure that the SDKInformation interface has been correctly filled out by the client
//
function verifySdkInfoPropertiesSet(interfaceInfo) {
    assert.equal(interfaceInfo.interfaces.urn_azureiot_Client_SDKInformation.properties.vendor.reported.value, "Microsoft")
    assert.equal(interfaceInfo.interfaces.urn_azureiot_Client_SDKInformation.properties.language.reported.value, "C")
    assert.notEqual(interfaceInfo.interfaces.urn_azureiot_Client_SDKInformation.properties.version.reported.value, null)
    // We don't currently check version field, as this can change over time.
}

//
// veryAllInterfacesAreRegistered makes sure that the e2e test interfaces were successfully registered.  These are stored
// in properties of the modelDiscovery interface
//
function veryAllInterfacesAreRegistered(done) {
    const client:DigitalTwinServiceClient = GetDigitalTwinServiceClient()

    client.digitalTwin.getAllInterfaces(testDeviceInfo.deviceId, function(error, interfaceInfo) {
        if (error) {
            console.error("Get digital twin interfaces failed")
            done(error)
        }
        else {
            console.log(interfaceInfo)

            var interfaces = interfaceInfo.interfaces.urn_azureiot_ModelDiscovery_DigitalTwin.properties.modelInformation.reported.value.interfaces

            assert.equal(interfaces.testCommands, testCommandInterfaceId)
            assert.equal(interfaces.testProperties, testPropertyInterfaceId)
            assert.equal(interfaces.testTelemetry, testTelemetryInterfaceID)
            assert.equal(interfaces.urn_azureiot_Client_SDKInformation, sdkInformationInterfaceId)
            assert.equal(interfaces.urn_azureiot_ModelDiscovery_ModelInformation, modelDiscoveryInterfaceId)
            verifySdkInfoPropertiesSet(interfaceInfo)
            done()
        }
    })
}

var argv = require('yargs')
    .usage('Usage: $0 --connectionstring <HUB CONNECTION STRING> --testexe <PATH OF EXECUTABLE TO HOST C SDK TESTS>')
    .option('connectionstring', {
        alias: 'c',
        describe: 'The connection string for the *IoTHub* instance to run tests against',
        type: 'string',
        demandOption: false
    })
    .option('testexe', {
        alias: 'e',
        describe: 'The full path of the application to execute',
        type: 'string',
        demandOption: true
    })
    .argv;

// testHubConnectionString is the Hub connection string used to create our test device against
var testHubConnectionString:string = getTestHubConnectionString()

//
// The global `before` clause is invoked before other tests are invoked
//
before("Initial test device and test child  process creation", createTestDeviceAndTestProcess)

//
// The global `after` clause is invoked as the final test
//
after("Free resources on test completion", deleteDeviceAndEndProcess)

//
// Tests for DigitalTwin commands
//
describe("DigitalTwin Command E2E tests", function() {
    var commandTest1 = new CommandTestData_TestCommands(`PayloadRequestResponse1`, `"ExpectedPayload for DT_E2E_PayloadRequestResponse1"`, 200, `"DT_E2E_PayloadRequestResponse1\'s response"`)
    var commandTest2 = new CommandTestData_TestCommands(`PayloadRequestResponse2`, `{"SlightlyMoreComplexRequestJson":{"SomethingEmbedded":[1,2,3,4]}}`, 200, `{"SlightlyMoreComplexResponseJson":{"SomethingEmbedded":[5,6,7,8]}}`)

    it("Run test commands1", function runTest1(done) {
        runTestCommand(commandTest1, 0, done)
    })

    it("Run test commands2", function runTest2(done) {
        runTestCommand(commandTest2, 0, done)
    })
})

//
// Tests for DigitalTwin telemetry
//
describe("DigitalTwin Telemetry E2E tests", function() {
    var telemetryTest1 = new CommandTestData_TestTelemetry('SendTelemetryMessage', "123", 200, `"Successfully Queued Telemetry message"`)
    var telemetryAckVerify = new CommandTestData_TestTelemetry('VerifyTelemetryAcknowledged', "3", 200, `"All telemetry messages sent have been acknowledged"`)

    it("Cause telemetry to be invoked (initial message)", function runTelemetryTest(done) {
        runTestCommand(telemetryTest1, 0, done)
    })

    it("Cause telemetry to be invoked (second message)", function runTelemetryTest(done) {
        runTestCommand(telemetryTest1, 0, done)
    })

    it("Cause telemetry to be invoked (third message)", function runTelemetryTest(done) {
        runTestCommand(telemetryTest1, 0, done)
    })

    it("Check telemetry messages ack'd", function checkTelemetryMessagesAckd(done) {
        runTestCommand(telemetryAckVerify, 0, done)
    })
})

//
// Tests for DigitalTwin properties
//
describe("DigitalTwin Properties E2E tests", function() {
    var propertyTest1 = new CommandTestData_PropertyTelemetry('SendReportedProperty1', "123", 200, `"Successfully processed property test request."`)
    var verifyProperty1Ackd = new CommandTestData_PropertyTelemetry('VerifyPropertiesAcknowledged', "123", 200, `"All reported properties from device have been acknowledged"`)

    it("Send reported property one", function runSendProperty1(done) {
        runTestCommand(propertyTest1, 0, done)
    })

    it("Send reported property one", function runSendProperty1(done) {
        runTestCommand(verifyProperty1Ackd, 0, done)
    })

    it("Get Digital Twin Interfaces", function runVerifyPropExpected(done) {
        verifyPropertySet(done, testPropertyInterfaceName, "SendProperty1_Name", `SendProperty1_Data`, 0)
    })

    it("Update properties #1", function runUpdateProperty(done) {
        updateProperties(done)
    })

    it("Get Digital Twin Interfaces", function runVerifyPropExpected(done) {
        verifyPropertySet(done, testPropertyInterfaceName, "ProcessUpdatedProperty1", `ValueOfPropertyBeingSet_1`, 0)
    })

    it("Verify all interfaces registered", function(done) {
        veryAllInterfacesAreRegistered(done)
    })
})
