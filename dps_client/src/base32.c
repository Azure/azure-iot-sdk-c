// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/gballoc.h"

#include "azure_hub_modules/base32.h"

static const char BASE32_VALUES[] = "abcdefghijklmnopqrstuvwxyz234567=";
#define TARGET_BLOCK_SIZE       5
#define INVALID_CHAR_POS        260

static size_t base32_encoding_length(size_t src_len)
{
    return (((src_len + TARGET_BLOCK_SIZE - 1) / TARGET_BLOCK_SIZE) * 8);
}

static size_t get_char_position(char pos_char)
{
    size_t result;
    // Validate characters
    if (pos_char >= '2' && pos_char <= '7')
    {
        result = 25+(pos_char - '2');
    }
    else if (pos_char >= 'a' && pos_char <= 'z')
    {
        result = (pos_char - 'a');
    }
    else
    {
        result = INVALID_CHAR_POS;
    }
    return result;
}

static BUFFER_HANDLE base32_decode_impl(const char* source)
{
    BUFFER_HANDLE result;
    char target_char;
    const char* iterator = source;

    while (*iterator != '\0')
    {
        target_char = (char)tolower((char)*iterator);
        // Get the positition of the char in base32
        size_t pos = get_char_position(target_char);
        if (pos == INVALID_CHAR_POS)
        {
            LogError("Invalid character specified");
            result = NULL;
            break;
        }

        iterator++;
    }
    return result;
}

static char* base32_encode_impl(const unsigned char* source, size_t src_size)
{
    char* result;

    // Allocate target buffer
    size_t output_len = base32_encoding_length(src_size);
    /* Codes_SRS_BASE32_07_009: [ base32_encode_impl shall allocate the buffer to the size of the encoding value. ] */
    if ((result = (char*)malloc(output_len + 1)) == NULL)
    {
        LogError("Failure allocating output buffer");
    }
    else
    {
        const unsigned char* iterator = source;
        size_t block_len = 0;
        size_t result_len = 0;
        unsigned char pos1 = 0;
        unsigned char pos2 = 0;
        unsigned char pos3 = 0;
        unsigned char pos4 = 0;
        unsigned char pos5 = 0;
        unsigned char pos6 = 0;
        unsigned char pos7 = 0;
        unsigned char pos8 = 0;

        memset(result, 0, output_len + 1);

        // Go through the source buffer sectioning off blocks of 5
        /* Codes_SRS_BASE32_07_010: [ base32_encode_impl shall look through source and separate each block into 5 bit chunks ] */
        while (src_size >= 1 && result != NULL)
        {
            pos1 = pos2 = pos3 = pos4 = pos5 = pos6 = pos7 = pos8 = 0;
            block_len = src_size > TARGET_BLOCK_SIZE ? TARGET_BLOCK_SIZE : src_size;
            // Fall through switch block to process the 5 (or smaller) block
            switch (block_len)
            {
            case 5:
                pos8 = (iterator[4] & 0x1f);
                pos7 = ((iterator[4] & 0xe0) >> 5);
            case 4:
                pos7 |= ((iterator[3] & 0x03) << 3);
                pos6 = ((iterator[3] & 0x7c) >> 2);
                pos5 = ((iterator[3] & 0x80) >> 7);
            case 3:
                pos5 |= ((iterator[2] & 0x0f) << 1);
                pos4 = ((iterator[2] & 0xf0) >> 4);
            case 2:
                pos4 |= ((iterator[1] & 0x01) << 4);
                pos3 = ((iterator[1] & 0x3e) >> 1);
                pos2 = ((iterator[1] & 0xc0) >> 6);
            case 1:
                pos2 |= ((iterator[0] & 0x07) << 2);
                pos1 = ((iterator[0] & 0xf8) >> 3);
                break;
            }
            // Move the iterator the block size
            iterator += block_len;
            // and decrement the src_size;
            src_size -= block_len;

            /* Codes_SRS_BASE32_07_012: [ If the src_size is not divisible by 8, base32_encode_impl shall pad the remaining places with =. ] */
            switch (block_len)
            {
                case 1: pos3 = pos4 = 32;
                case 2: pos5 = 32;
                case 3: pos6 = pos7 = 32;
                case 4: pos8 = 32;
                case 5:
                    break;
            }

            /* Codes_SRS_BASE32_07_011: [ base32_encode_impl shall then map the 5 bit chunks into one of the BASE32 values (a-z,2,3,4,5,6,7) values. ] */
            result[result_len++] = BASE32_VALUES[pos1];
            result[result_len++] = BASE32_VALUES[pos2];
            result[result_len++] = BASE32_VALUES[pos3];
            result[result_len++] = BASE32_VALUES[pos4];
            result[result_len++] = BASE32_VALUES[pos5];
            result[result_len++] = BASE32_VALUES[pos6];
            result[result_len++] = BASE32_VALUES[pos7];
            result[result_len++] = BASE32_VALUES[pos8];
        }
    }
    return result;
}

BUFFER_HANDLE Base32_Decoder(const char* source)
{
    BUFFER_HANDLE result;
    if (source == NULL)
    {
        LogError("invalid parameter const char* source=%p", source);
        result = NULL;
    }
    else
    {
        LogError("Not Implemented");
        result = NULL;
    }
    return result;
}

char* Base32_Encode_Bytes(const unsigned char* source, size_t size)
{
    char* result;
    if (source == NULL)
    {
        /* Codes_SRS_BASE32_07_004: [ If source is NULL Base32_Encoder shall return NULL. ] */
        result = NULL;
        LogError("Failure: Invalid input parameter source");
    }
    else if (size == 0)
    {
        /* Codes_SRS_BASE32_07_005: [ If size is 0 Base32_Encoder shall return an empty string. ] */
        result = malloc(1);
        strcpy(result, "");
    }
    else
    {
        /* Codes_SRS_BASE32_07_007: [ Base32_Encode_Bytes shall call into base32_encoder_impl to encode the source data. ] */
        result = base32_encode_impl(source, size);
        if (result == NULL)
        {
            /* Codes_SRS_BASE32_07_014: [ Upon failure Base32_Encode_Bytes shall return NULL. ] */
            LogError("encoding of unsigned char failed.");
        }
    }
    /* Codes_SRS_BASE32_07_006: [ If successful Base32_Encoder shall return the base32 value of input. ] */
    return result;
}

STRING_HANDLE Base32_Encoder(BUFFER_HANDLE source)
{
    STRING_HANDLE result;
    if (source == NULL)
    {
        /* Codes_SRS_BASE32_07_001: [ If source is NULL Base32_Encoder shall return NULL. ] */
        result = NULL;
        LogError("Failure: Invalid input parameter");
    }
    else
    {
        size_t input_len = BUFFER_length(source);
        const unsigned char* input_value = BUFFER_u_char(source);
        if (input_value == NULL || input_len == 0)
        {
            /* Codes_SRS_BASE32_07_015: [ If size is 0 Base32_Encoder shall return an empty string. ] */
            result = STRING_new();
            if (result == NULL)
            {
                LogError("Failure constructing new string.");
            }
        }
        else
        {
            /* Codes_SRS_BASE32_07_003: [ Base32_Encoder shall call into base32_encoder_impl to encode the source data. ] */
            char* encoded = base32_encode_impl(input_value, input_len);
            if (encoded == NULL)
            {
                /* Codes_SRS_BASE32_07_014: [ Upon failure Base32_Encoder shall return NULL. ] */
                LogError("encoding failed.");
                result = NULL;
            }
            else
            {
                /* Codes_SRS_BASE32_07_012: [ Base32_Encoder shall wrap the base32_encoder_impl result into a STRING_HANDLE. ] */
                result = STRING_construct(encoded);
                if (result == NULL)
                {
                    /* Codes_SRS_BASE32_07_014: [ Upon failure Base32_Encoder shall return NULL. ] */
                    LogError("string construction failed.");
                }
                free(encoded);
            }
        }
    }
    /* Codes_SRS_BASE32_07_002: [ If successful Base32_Encoder shall return the base32 value of source. ] */
    return result;
}
