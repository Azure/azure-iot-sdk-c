// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/* This is a template file used for porting */

/* TODO: If all the below adapters call the standard C functions, please simply use the agentime.c that already exists. */

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <time.h>
#include "mico.h"
#include "gballoc.h"
#include "agenttime.h"

time_t get_time(time_t* currentTime)
{
    mico_utc_time_t utime;

    mico_time_get_utc_time( &utime );
    return (time_t)utime;
}

double get_difftime(time_t stopTime, time_t startTime)
{
    return difftime(stopTime, startTime);
}


struct tm* get_gmtime(time_t* currentTime)
{
    return localtime(currentTime);
}

char* get_ctime(time_t* timeToGet)
{
    return ctime(timeToGet);
}

