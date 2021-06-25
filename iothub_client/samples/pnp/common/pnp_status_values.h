// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PNP_STATUS_VALUES_H
#define PNP_STATUS_VALUES_H

// By convention, PnP uses HTTP status codes to indicate success and error states even for 
// non-HTTP based protocols.  The codes below are the ones the sample uses, but applications
// may use additional ones.

#define PNP_STATUS_SUCCESS 200
#define PNP_STATUS_BAD_FORMAT 400
#define PNP_STATUS_NOT_FOUND 404
#define PNP_STATUS_INTERNAL_ERROR 500

#endif /* PNP_STATUS_VALUES_H */

