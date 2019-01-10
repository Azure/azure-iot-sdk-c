# C-SDK Resource Information

Detailed here are the captured the C SDK Resource constraints for each protocol.

All test are executed on an ubuntu 18.04.1 image with GCC version 7.3.0 using C-SDK version 1.2.12.

## On Disk Binary Information

This is the size of the binary built with the c sdk for each protocol.  The application sets up the SDK and sends 1 telemetry message.

### C-SDK without upload to blob

CMAKE Command: cmake -Ddont_use_uploadtoblob=ON ..

| Protocol                | Size In Bytes
|-------------------------|---------------------
| MQTT                    | 303,496
| MQTT With Web Sockets   | 343,244
| AMQP                    | 775,816
| AMQP With Web Sockets   | 815,824

### C-SDK without upload to blob and logging turned off

CMAKE Command: cmake -Ddont_use_uploadtoblob=ON -Dno_logging=ON ..

| Protocol                | Size In Bytes
|-------------------------|---------------------
| MQTT                    | 204,088
| MQTT With Web Sockets   | 223,640
| AMQP                    | 479,120
| AMQP With Web Sockets   | 498,672

### C-SDK using strip command

CMAKE Command: cmake -Ddont_use_uploadtoblob=ON -Dno_logging=ON ..
strip [executable path]

| Protocol                | Size In Bytes
|-------------------------|---------------------
| MQTT                    | 167,552
| MQTT With Web Sockets   | 183,936
| AMQP                    | 397,464
| AMQP With Web Sockets   | 413,864

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
