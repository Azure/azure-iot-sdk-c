// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOS_SAMPLE_H
#define IOS_SAMPLE_H

#ifdef __cplusplus
extern "C"
{
#endif

int printf_to_sample_window(const char* format, ...);
int fake_getchar(void);
int fake_IoTHubClient_LL_SetOption(void* iotHubClientHandle, const char* optionName, const void* value);

#define printf printf_to_sample_window
#define IoTHubClient_LL_SetOption(a,b,c) fake_IoTHubClient_LL_SetOption(a,b,c)
#define ThreadAPI_Sleep(a) ThreadAPI_Sleep(20)

#ifdef __cplusplus
}
#endif

#endif /* IOS_SAMPLE_H */
