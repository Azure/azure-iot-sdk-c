// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SAMPLE_INIT_H
#define SAMPLE_INIT_H
#if defined(ARDUINO_ARCH_ESP8266)
  #define sample_init esp8266_sample_init
  void esp8266_sample_init(const char* ssid, const char* password);
#endif

#endif // SAMPLE_INIT_H
