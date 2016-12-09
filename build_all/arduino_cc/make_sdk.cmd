@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@echo off

REM This script creates the SDK folder with the latest bits from the Azure SDK repository.
REM It removes some files we do not need.  It currently targets the develop branch until fixes are
REM merged into master.

if "%1" equ "" (
    set Work_path=%~dp0..\..\..\Work
) else (
    set Work_path=%1
)
rem // resolve to fully qualified path
for %%i in ("%Work_path%") do set Work_path=%%~fi

set Libraries_path=%Work_path%\arduino\libraries\
set AzureIoTHub_path=%Libraries_path%AzureIoTHub\
set AzureIoTProtocolHTTP_path=%Libraries_path%AzureIoTProtocol_HTTP\
set AzureIoTProtocolMQTT_path=%Libraries_path%AzureIoTProtocol_MQTT\
set AzureIoTProtocolAMQP_path=%Libraries_path%AzureIoTProtocol_AMQP\
set AzureIoTUtility_path=%Libraries_path%AzureIoTUtility\
set AzureUHTTP_path=%AzureIoTProtocolHTTP_path%src\azure_uhttp_c\
set AzureUMQTT_path=%AzureIoTProtocolMQTT_path%src\azure_umqtt_c\
set AzureUAMQP_path=%AzureIoTProtocolAMQP_path%src\azure_uamqp_c\
set SharedUtility_path=%AzureIoTUtility_path%src\azure_c_shared_utility\
set Adapters_path=%AzureIoTUtility_path%src\adapters\
set sdk_path=%AzureIoTHub_path%src\sdk\
set AzureIoTSDKs_path=%~dp0..\..\
rem // resolve to fully qualified path
for %%i in ("%AzureIoTSDKs_path%") do set AzureIoTSDKs_path=%%~fi

mkdir %Libraries_path%
pushd %Libraries_path%

if exist "%AzureIoTHub_path%" rd /s /q %AzureIoTHub_path%

robocopy %~dp0\base-libraries\AzureIoTHub %AzureIoTHub_path% -MIR
robocopy %~dp0\base-libraries\AzureIoTUtility %AzureIoTUtility_path% -MIR
robocopy %~dp0\base-libraries\AzureIoTProtocol_HTTP %AzureIoTProtocolHTTP_path% -MIR
robocopy %~dp0\base-libraries\AzureIoTProtocol_MQTT %AzureIoTProtocolMQTT_path% -MIR

mkdir %sdk_path%

cd /D %AzureIoTSDKs_path%
echo Upstream HEAD @ > %sdk_path%metadata.txt
git rev-parse HEAD >> %sdk_path%metadata.txt

copy %AzureIoTSDKs_path%LICENSE %AzureIoTHub_path%LICENSE

copy %AzureIoTSDKs_path%iothub_client\src\ %sdk_path%
copy %AzureIoTSDKs_path%iothub_client\inc\ %sdk_path%
copy %AzureIoTSDKs_path%serializer\src\ %sdk_path%
copy %AzureIoTSDKs_path%serializer\inc\ %sdk_path%
copy %AzureIoTSDKs_path%parson\parson.* %sdk_path%
copy %AzureIoTSDKs_path%serializer\samples\simplesample_http\simplesample_http.* %AzureIoTHub_path%examples\simplesample_http
copy %AzureIoTSDKs_path%serializer\samples\simplesample_http\simplesample_http.* %AzureIoTProtocolHTTP_path%examples\simplesample_http
copy %AzureIoTSDKs_path%serializer\samples\simplesample_mqtt\simplesample_mqtt.* %AzureIoTProtocolMQTT_path%examples\simplesample_mqtt

mkdir %SharedUtility_path%
mkdir %Adapters_path%
copy %AzureIoTSDKs_path%c-utility\inc\azure_c_shared_utility %SharedUtility_path%
copy %AzureIoTSDKs_path%c-utility\src\ %SharedUtility_path%

copy %AzureIoTSDKs_path%c-utility\adapters\agenttime.c %Adapters_path%
copy %AzureIoTSDKs_path%c-utility\adapters\tickcounter_tirtos.c %Adapters_path%
copy %AzureIoTSDKs_path%c-utility\adapters\*arduino.* %Adapters_path%

mkdir %AzureUHTTP_path%
copy %AzureIoTSDKs_path%c-utility\adapters\httpapi_compact.c %AzureUHTTP_path%

mkdir %AzureUMQTT_path%
copy %AzureIoTSDKs_path%umqtt\src %AzureUMQTT_path%
mkdir %AzureIoTHub_path%src\azure_umqtt_c\
copy %AzureIoTSDKs_path%umqtt\inc\azure_umqtt_c %AzureIoTHub_path%src\azure_umqtt_c\

del %sdk_path%*amqp*.*
del %sdk_path%iothubtransportmqtt_websockets.*

del %SharedUtility_path%tlsio_*.*
del %SharedUtility_path%wsio*.*
del %SharedUtility_path%x509_*.*

popd
