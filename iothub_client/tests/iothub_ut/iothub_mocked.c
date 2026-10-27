// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/* iothub.c with c-logging's logger_init/logger_deinit replaced by mocks (the test runner needs the real ones). */
int mock_logger_init(void);
void mock_logger_deinit(void);
#define IOTHUB_LOGGER_INIT() mock_logger_init()
#define IOTHUB_LOGGER_DEINIT() mock_logger_deinit()

#include "../../src/iothub.c"
