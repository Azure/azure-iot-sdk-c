// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "cbor.h"
#include <cjson/cJSON.h>

CborError decode_json(cJSON *json, CborEncoder *encoder);

CborError cbor_parse_recursive(CborValue *it, int nesting_level);
