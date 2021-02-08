// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "cbor_helper.h"

static void dumpbytes(const uint8_t *buf, size_t len)
{
    while (len--)
        printf("%02X ", *buf++);
}

static void indent(int nestingLevel)
{
    while (nestingLevel--)
        puts("  ");
}

static size_t get_cjson_size_limited(cJSON *container)
{
    // cJSON_GetArraySize is O(n), so don't go too far
    unsigned s = 0;
    cJSON *item;
    for (item = container->child; item; item = item->next) {
        if (++s > 255)
            return CborIndefiniteLength;
    }
    return s;
}

CborError decode_json(cJSON *json, CborEncoder *encoder)
{
    CborEncoder container;
    CborError err;
    cJSON *item;

    switch (json->type) {
    case cJSON_False:
    case cJSON_True:
        return cbor_encode_boolean(encoder, json->type == cJSON_True);

    case cJSON_NULL:
        return cbor_encode_null(encoder);

    case cJSON_Number:
        if ((double)json->valueint == json->valuedouble)
            return cbor_encode_int(encoder, json->valueint);
encode_double:
        // the only exception that JSON is larger: floating point numbers
        container = *encoder;   // save the state
        err = cbor_encode_double(encoder, json->valuedouble);

        // if (err == CborErrorOutOfMemory) {
        //     buffersize += 1024;
        //     uint8_t *newbuffer = realloc(buffer, buffersize);
        //     if (newbuffer == NULL)
        //         return err;

        //     *encoder = container;   // restore state
        //     encoder->data.ptr = newbuffer + (container.data.ptr - buffer);
        //     encoder->end = newbuffer + buffersize;
        //     buffer = newbuffer;
        //     goto encode_double;
        // }
        return err;

    case cJSON_String:
        return cbor_encode_text_stringz(encoder, json->valuestring);

    default:
        return CborErrorUnknownType;

    case cJSON_Array:
        err = cbor_encoder_create_array(encoder, &container, get_cjson_size_limited(json));
        if (err)
            return err;
        for (item = json->child; item; item = item->next) {
            err = decode_json(item, &container);
            if (err)
                return err;
        }
        return cbor_encoder_close_container_checked(encoder, &container);

    case cJSON_Object:
        err = cbor_encoder_create_map(encoder, &container, get_cjson_size_limited(json));
        if (err)
            return err;

        for (item = json->child ; item; item = item->next) {
            err = cbor_encode_text_stringz(&container, item->string);
            if (err)
                return err;

            err = decode_json(item, &container);
            if (err)
                return err;
        }

        return cbor_encoder_close_container_checked(encoder, &container);
    }
}

CborError cbor_parse_recursive(CborValue *it, int nesting_level)
{
    while (!cbor_value_at_end(it)) {
        CborError err;
        CborType type = cbor_value_get_type(it);

        indent(nesting_level);
        switch (type) {
        case CborArrayType:
        case CborMapType: {
            // recursive type
            CborValue recursed;
            assert(cbor_value_is_container(it));
            puts(type == CborArrayType ? "Array[" : "Map[");
            err = cbor_value_enter_container(it, &recursed);
            if (err)
                return err;       // parse error
            err = cbor_parse_recursive(&recursed, nesting_level + 1);
            if (err)
                return err;       // parse error
            err = cbor_value_leave_container(it, &recursed);
            if (err)
                return err;       // parse error
            indent(nesting_level);
            puts("]");
            continue;
        }

        case CborIntegerType: {
            int64_t val;
            cbor_value_get_int64(it, &val);     // can't fail
            printf("%lld\n", (long long)val);
            break;
        }

        case CborByteStringType: {
            uint8_t *buf;
            size_t n;
            err = cbor_value_dup_byte_string(it, &buf, &n, it);
            if (err)
                return err;     // parse error
            dumpbytes(buf, n);
            puts("");
            free(buf);
            continue;
        }

        case CborTextStringType: {
            char *buf;
            size_t n;
            err = cbor_value_dup_text_string(it, &buf, &n, it);
            if (err)
                return err;     // parse error
            puts(buf);
            free(buf);
            continue;
        }

        case CborTagType: {
            CborTag tag;
            cbor_value_get_tag(it, &tag);       // can't fail
            printf("Tag(%lld)\n", (long long)tag);
            break;
        }

        case CborSimpleType: {
            uint8_t type;
            cbor_value_get_simple_type(it, &type);  // can't fail
            printf("simple(%u)\n", type);
            break;
        }

        case CborNullType:
            puts("null");
            break;

        case CborUndefinedType:
            puts("undefined");
            break;

        case CborBooleanType: {
            bool val;
            cbor_value_get_boolean(it, &val);       // can't fail
            puts(val ? "true" : "false");
            break;
        }

        case CborDoubleType: {
            double val;
            if (false) {
                float f;
        case CborFloatType:
                cbor_value_get_float(it, &f);
                val = f;
            } else {
                cbor_value_get_double(it, &val);
            }
            printf("%g\n", val);
            break;
        }
        case CborHalfFloatType: {
            uint16_t val;
            cbor_value_get_half_float(it, &val);
            printf("__f16(%04x)\n", val);
            break;
        }

        case CborInvalidType:
            assert(false);      // can't happen
            break;
        }

        err = cbor_value_advance_fixed(it);
        if (err)
            return err;
    }
    return CborNoError;
}
