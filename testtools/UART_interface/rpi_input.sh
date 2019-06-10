#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

# basic setup of input script for python
echo -e "$RASPI_USER\n$RASPI_PWD\nsz -a $RASPI_TEST_FILE > $RASPI_PORT < $RASPI_PORT\n\n$RASPI_SERIAL_TASK\n\n" | cat > input.txt
