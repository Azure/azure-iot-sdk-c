# C-SDK Resource Information

Detailed here capture the C SDK Resource consumption for each protocol.

All tests are executed on an ubuntu 18.04.1 docker container with GCC version 7.3.0 using C-SDK version 1.2.12.

## On Disk Binary Size

This is the size of the binary built with the c sdk for each protocol.  The application sets up the SDK and sends 1 telemetry message.

### C-SDK without upload to blob

CMAKE Command:

```Shell
cmake -DCMAKE_BUILD_TYPE=Release -Ddont_use_uploadtoblob=ON ..
strip [executable path]
```

| Protocol                | Size In Bytes
|-------------------------|---------------------
| MQTT                    | 303,496
| MQTT With Web Sockets   | 343,244
| AMQP                    | 775,816
| AMQP With Web Sockets   | 815,824

### C-SDK without upload to blob and logging turned off

CMAKE Command:

```Shell
cmake -DCMAKE_BUILD_TYPE=Release -Ddont_use_uploadtoblob=ON -Dno_logging=ON ..
strip [executable path]
```

| Protocol                | Size In Bytes
|-------------------------|---------------------
| MQTT                    | 204,088
| MQTT With Web Sockets   | 223,640
| AMQP                    | 479,120
| AMQP With Web Sockets   | 498,672

### C-SDK using strip command

CMAKE Command:

```Shell
cmake -DCMAKE_BUILD_TYPE=Release -Ddont_use_uploadtoblob=ON -Dno_logging=ON ..
strip [executable path]
```

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

- `Max Memory Bytes` represents all c-sdk calls to malloc.  Memory allocated on the stack is not captured

## Network traffic Information

Network packets measure the number of bytes that are sent form the C-SDK with the specified payload size.  This value excludes the connection and disconnection headers.

| Protocol                | Payload Size | Sent Bytes | Send Calls | Recv Bytes | Recv calls
|-------------------------|--------------|------------|------------|------------|------------
| MQTT                    | 1024         | 1,242      | 2          |  69        | 2
| MQTT With Web Sockets   | 1024         | 1,242      | 2          |  69        | 2
| AMQP                    | 1024         | 1,720      | 8          |  85        | 2
| AMQP With Web Sockets   | 1024         | 1,768      | 8          |  85        | 2

- `Bytes Send` is the amount of data written by the c-sdk on the socket.
- `Send Calls`/`Recv calls` are the number of types the c-sdk calls send or recv on the socket
