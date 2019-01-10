# C-SDK Resource Information

Detailed here are the captured the C SDK Resource constraints for each protocol.

All test are executed on an ubuntu 18.04.1 image with GCC version 7.3.0 using C-SDK version 1.2.12.

## On Disk Binary Information

This is the size of the binary built with the c sdk for each protocol.  The appication setups the SDK and sends 1 telemetry message.

| Protocol                | Size In Bytes
|-------------------------|---------------------
| MQTT                    | 303,496
| MQTT With Web Sockets   | 343,244
| AMQP                    | 775,816
| AMQP With Web Sockets   | 815,824

## Runtime Memory Information

The runtime Memory information is the measurement of the C SDK memory allocation when sending 1 telemetry message

| Protocol                | Messages Sent | Max Memory Bytes | Number Allocations
|-------------------------|---------------|------------------|--------------------
| MQTT                    |      1        |      15,058      | 172
| MQTT With Web Sockets   |      1        |      15,753      | 234
| AMQP                    |      1        |      27,335      | 1,676
| AMQP With Web Sockets   |      1        |      28,809      | 1,882

## Network traffic Information

Network packets measure the number of bytes that are sent form the C-SDK with the specified payload size.  This value excludes the connection and disconnection headers.

| Protocol                | Messages Payload | Bytes Sent | Number Sends | Received Bytes | Number Received
|-------------------------|------------------|------------|--------------|----------------|------------------
| MQTT                    | 1024             | 1,242      | 2            |  69            | 2
| MQTT With Web Sockets   | 1024             | 1,242      | 2            |  69            | 2
| AMQP                    | 1024             | 1,720      | 8            |  85            | 2
| AMQP With Web Sockets   | 1024             | 1,768      | 8            |  85            | 2
