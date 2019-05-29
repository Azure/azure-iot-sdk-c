#!/bin/bash

# single quotes are used to turn off special meaning of $ in variables.
export IOTHUB_CONNECTION_STRING=''
export IOTHUB_EVENTHUB_CONNECTION_STRING=''
export IOTHUB_EVENTHUB_LISTEN_NAME=''
export IOTHUB_E2E_X509_THUMBPRINT=''
export IOTHUB_E2E_X509_CERT_BASE64=''
export IOTHUB_E2E_X509_PRIVATE_KEY_BASE64=''
export IOTHUB_EVENTHUB_CONSUMER_GROUP='$Default'
IOTHUB_PARTITION_COUNT=16

/home/pi/myagent/run.sh