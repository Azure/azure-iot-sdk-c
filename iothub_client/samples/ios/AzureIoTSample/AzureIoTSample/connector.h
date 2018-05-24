// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//
#import <Foundation/Foundation.h>

#ifndef connector_h
#define connector_h

@class ViewController;

@protocol logTarget
- (void)addLogEntry: (NSString*)logEntry;
@end

void init_connector(id <logTarget> vc);

void run_ios_sample(void);

#endif /* connector_h */
