# Running the C SDK on Constrained Devices

IoT devices are coming with greater memory constrains and the IoTHub SDK has options to shrink its ROM and RAM footprint considerably when you invoke cmake with the correct flags.  These are some tips to reduce the c sdk memory footprints on these devices.

## Run with only MQTT

To maximize memory usage you should only include the protocol that is needed.  For constrained devices, we recommend using MQTT which provides the best balance between RAM/ROM usage and feature support.  To enable the c-sdk to run with just the mqtt protocol you will need to run the following cmake command:

```Shell
cmake -Duse_amqp=OFF -Duse_http=OFF <Path_to_cmake>
```

## Run without logging

There is extensive logging throughout the SDK.  Every message will be included in the SDK's RAM and ROM footprint.  Removing logging will reduce the size of your application.

```Shell
cmake -Duse_amqp=OFF -Duse_http=OFF -Dno_logging=ON <Path_to_cmake>
```

## Running the SDK without upload to blob

Upload to blob is an SDK feature used to send data to Azure Storage.  If your application doesn't need this feature you can remove the files

```Shell
cmake -Duse_amqp=OFF -Duse_http=OFF -Dno_logging=ON -Ddont_use_uploadtoblob=ON <Path_to_cmake>
```

## Running strip on Linux environment

The [strip](https://en.wikipedia.org/wiki/Strip_(Unix)) command is used to reduce the size of binaries on the linux systems.  After you compile your application use strip to reduce the size of the final application.

```Shell
strip -s <Path_to_executable>
```
