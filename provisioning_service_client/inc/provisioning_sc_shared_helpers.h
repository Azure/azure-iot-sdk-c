// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PROVISIONING_SC_SHARED_HELPERS_H
#define PROVISIONING_SC_SHARED_HELPERS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "parson.h"

#define NECESSITY_VALUES    \
    REQUIRED,               \
    OPTIONAL                \

DEFINE_ENUM(NECESSITY, NECESSITY_VALUES);

typedef JSON_Value*(*TO_JSON_FUNCTION)(void* handle);
typedef void*(*FROM_JSON_FUNCTION)(JSON_Object* root_object);

MOCKABLE_FUNCTION(, int, mallocAndStrcpy_overwrite, char**, dest, const char*, source);
MOCKABLE_FUNCTION(, int, copy_json_string_field, char**, dest, JSON_Object*, root_object, const char*, json_key);
MOCKABLE_FUNCTION(, int, json_serialize_and_set_struct, JSON_Object*, root_object, const char*, json_key, void*, structure, TO_JSON_FUNCTION, toJson, NECESSITY, necessity);
MOCKABLE_FUNCTION(, int, json_deserialize_and_get_struct, void**, dest, JSON_Object*, root_object, const char*, json_key, FROM_JSON_FUNCTION, fromJson, NECESSITY, necessity);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PROVISIONING_SC_SHARED_HELPERS_H */