// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <azure_c_shared_utility/xlogging.h>
#import "connector.h"
#import <stdarg.h>
#import "iothub_sample.h"

static id <logTarget> log_target = nil;

static void ios_consolelogger_log(LOG_CATEGORY log_category, const char* file, const char* func, int line, unsigned int options, const char* format, ...)
{
    const size_t buffer_size = 1024;
    char buffer[buffer_size];
    va_list args;
    va_start(args, format);
    NSMutableString *temp = [[NSMutableString alloc]initWithCapacity:256];
    
    time_t t = time(NULL);
    
    switch (log_category)
    {
        case AZ_LOG_INFO:
            [temp appendString:@"Info: "];
            break;
        case AZ_LOG_ERROR:
            (void)snprintf(buffer, buffer_size, "Error: Time:%.24s File:%s Func:%s Line:%d ", ctime(&t), file, func, line);
            [temp appendFormat:@"%s", buffer];
            break;
        default:
            break;
    }
    
    (void)vsnprintf(buffer, buffer_size, format, args);
    [temp appendFormat:@"%s", buffer];
    va_end(args);
    
    (void)log_category;
    if (options & LOG_LINE)
    {
        [temp appendString:@"\n"];
    }
    [log_target addLogEntry:temp];
}

void init_connector(id <logTarget> vc)
{
    log_target = vc;
    xlogging_set_log_function(ios_consolelogger_log);
    (void)run_iothub_sample();
}

int fake_IoTHubClient_LL_SetOption(void* iotHubClientHandle, const char* optionName, const void* value)
{
    return 0;
}
