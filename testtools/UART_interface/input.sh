#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

# basic setup of input script for python
echo -e "help\r\nset_az_iothub $IOTHUB_CONNECTION_STRING\r\n\r\n$SERIAL_TASK\r\n\r\nversion" | cat > input.txt
