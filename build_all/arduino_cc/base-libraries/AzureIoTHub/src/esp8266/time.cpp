// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#if defined(ARDUINO_ARCH_ESP8266)
#include <time.h>

extern "C" {
    double difftime(time_t _time2, time_t _time1)
    {
        return (double)_time2 - (double)_time1;
    }
    
    size_t strftime(char *s, size_t maxsize, const char* format, const struct tm *timeptr)
    {
        /*For now esp8266 will not report time.*/
        (void)(s, maxsize, format, timeptr);        
        return 0;
    }
}
#endif