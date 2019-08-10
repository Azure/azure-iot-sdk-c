# Properly format of Digital Twin data

All Digital Twin data payloads are currently based on JSON.  In particular, this JSON must be modeled in the Digital Twin Definition Language (DTDL) file that defines the schema for your interface.  This is inline with the Digital Twin modeling and service guidelines.  The JSON payloads that the device itself creates are specified in:

* Telemetry messages
* Reporting property values
* Setting a response message when processing a command
* Setting a payload when sending an asynchronous command update

**Not sending valid JSON or JSON that does not match the model defined will result in potentially very hard to diagnose problems for server backend developers.  Your application may not be notified it has sent bad content in most cases.**  Issues will only be apparent during service application / system integration.

The C SDK is *not* integrated into any modeling checkers to make sure the payload is matches the model.  The C SDK does not even make sure the payload is legal JSON.  Server ingestion currently does not check against the model and in many cases for legal JSON, either.

A particularly common mistake for C developers occurs in sending strings.  The `dataPayload` below is invalid JSON:

```c
const char* dataPayload = "Value";
```

For this `dataPayload` to be a valid JSON string, it needs quotes surrounding it.  Instead:

```c
const char* dataPayload = "\"Value\"";
```

What happens if you do not specify legal JSON depends on which scenario you're using:

* **Telemetry messages** with bad JSON will be successfully ingested by the server.  This means the device application's callback will **NOT** receive an error on sending invalid telemetry.  The JSON is not parsed for validity on ingestion to optimize server, as the amount of traffic it ingests such parsing would be create scaling problems.
* **Reporting property values** The server does parse the data for this scenario and the device application's callback will receive an error *only* if the JSON is not valid.  If the payload does not match the modeled schema but is legal JSON, then the server will successfully accept the request (which pushes the debugging problem to later).
* **Setting a response message when processing a command** There is no mechanism for applications to be notified that a command response has been successfully or unsuccessfully processed.  Therefore sending bad JSON will silently fail.
* **Setting a payload when sending an asynchronous command update** There is no mechanism for applications to be notified that an asynchronous command response has been successfully or unsuccessfully processed.  Therefore sending bad JSON will silently fail.
