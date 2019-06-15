#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

# basic setup of input script for python
echo -e "$RASPI_USER\n$RASPI_PWD\nrm $RASPI_TEST_FILE\nsz -a $RASPI_TEST_FILE > $RASPI_PORT < $RASPI_PORT\ncp $RASPI_TEST_FILE $RASPI_TEST_FILEclone\n./$RASPI_TEST_FILEclone\n\n$RASPI_SERIAL_TASK\n\n" | cat > input.txt
