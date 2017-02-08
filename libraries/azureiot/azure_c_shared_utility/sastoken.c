// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "gballoc.h"
#include "sastoken.h"
#include "urlencode.h"
#include "hmacsha256.h"
#include "base64.h"
#include "agenttime.h"
#include "strings.h"
#include "buffer_.h"
#include "xlogging.h"
#include "crt_abstractions.h"

static double getExpiryValue(const char* expiryASCII)
{
    double value = 0;
    size_t i = 0;
    for (i = 0; expiryASCII[i] != '\0'; i++)
    {
        if (expiryASCII[i] >= '0' && expiryASCII[i] <= '9')
        {
            value = value * 10 + (expiryASCII[i] - '0');
        }
        else
        {
            value = 0;
            break;
        }
    }
    return value;
}

bool SASToken_Validate(STRING_HANDLE sasToken)
{
    bool result;
    /*Codes_SRS_SASTOKEN_25_025: [**SASToken_Validate shall get the SASToken value by invoking STRING_c_str on the handle.**]***/
    const char* sasTokenArray = STRING_c_str(sasToken);

    /***Codes_SRS_SASTOKEN_25_024: [**If handle is NULL then SASToken_Validate shall return false.**] ***/
    /*Codes_SRS_SASTOKEN_25_026: [**If STRING_c_str on handle return NULL then SASToken_Validate shall return false.**]***/
    if (sasToken == NULL || sasTokenArray == NULL)
    {
        result = false;
    }
    else
    {
        int seStart = -1, seStop = -1;
        int srStart = -1, srStop = -1;
        int sigStart = -1, sigStop = -1;
        int tokenLength = (int) STRING_length(sasToken);
        int i ;
        for (i = 0; i < tokenLength; i++)
        {
            if (sasTokenArray[i] == 's' && sasTokenArray[i + 1] == 'e' && sasTokenArray[i + 2] == '=') // Look for se=
            {
                seStart = i + 3;
                if (srStart > 0 && srStop < 0)
                {
                    if (sasTokenArray[i - 1] != '&' && sasTokenArray[i - 1] == ' ') // look for either & or space
                        srStop = i - 1;
                    else if (sasTokenArray[i - 1] == '&')
                        srStop = i - 2;
                    else
                        seStart = -1; // as the format is not either "&se=" or " se="
                }
                else if (sigStart > 0 && sigStop < 0)
                {
                    if (sasTokenArray[i - 1] != '&' && sasTokenArray[i - 1] == ' ')
                        sigStop = i - 1;
                    else if (sasTokenArray[i - 1] == '&')
                        sigStop = i - 2;
                    else
                        seStart = -1;
                }
                continue;
            }
            if (sasTokenArray[i] == 's' && sasTokenArray[i + 1] == 'r' && sasTokenArray[i + 2] == '=') // Look for sr=
            {
                srStart = i + 3;
                if (seStart > 0 && seStop < 0)
                {
                    if (sasTokenArray[i - 1] != '&' && sasTokenArray[i - 1] == ' ')
                        seStop = i - 1;
                    else if (sasTokenArray[i - 1] == '&')
                        seStop = i - 2;
                    else
                        srStart = -1;
                }
                else if (sigStart > 0 && sigStop < 0)
                {
                    if (sasTokenArray[i - 1] != '&' && sasTokenArray[i - 1] == ' ')
                        sigStop = i - 1;
                    else if (sasTokenArray[i - 1] == '&')
                        sigStop = i - 2;
                    else
                        srStart = -1;
                }
                continue;
            }
            if (sasTokenArray[i] == 's' && sasTokenArray[i + 1] == 'i' && sasTokenArray[i + 2] == 'g' && sasTokenArray[i + 3] == '=') // Look for sig=
            {
                sigStart = i + 4;
                if (srStart > 0 && srStop < 0)
                {
                    if (sasTokenArray[i - 1] != '&' && sasTokenArray[i - 1] == ' ')
                        srStop = i - 1;
                    else if (sasTokenArray[i - 1] == '&')
                        srStop = i - 2;
                    else
                        sigStart = -1;
                }
                else if (seStart > 0 && seStop < 0)
                {
                    if (sasTokenArray[i - 1] != '&' && sasTokenArray[i - 1] == ' ')
                        seStop = i - 1;
                    else if (sasTokenArray[i - 1] == '&')
                        seStop = i - 2;
                    else
                        sigStart = -1;
                }
                continue;
            }
        }
        /*Codes_SRS_SASTOKEN_25_027: [**If SASTOKEN does not obey the SASToken format then SASToken_Validate shall return false.**]***/
        /*Codes_SRS_SASTOKEN_25_028: [**SASToken_validate shall check for the presence of sr, se and sig from the token and return false if not found**]***/
        if (seStart < 0 || srStart < 0 || sigStart < 0)
        {
            result = false;
        }
        else
        {
            if (seStop < 0)
            {
                seStop = tokenLength;
            }
            else if (srStop < 0)
            {
                srStop = tokenLength;
            }
            else if (sigStop < 0)
            {
                sigStop = tokenLength;
            }

            if ((seStop <= seStart) ||
                (srStop <= srStart) ||
                (sigStop <= sigStart))
            {
                result = false;
            }
            else
            {
                char* expiryASCII = malloc(seStop - seStart + 1);
                /*Codes_SRS_SASTOKEN_25_031: [**If malloc fails during validation then SASToken_Validate shall return false.**]***/
                if (expiryASCII == NULL)
                {
                    result = false;
                }
                else
                {
                    double expiry;
                    for (i = seStart; i < seStop; i++)
                    {
                        expiryASCII[i - seStart] = sasTokenArray[i];
                    }
                    expiryASCII[seStop - seStart] = '\0';

                    expiry = getExpiryValue(expiryASCII);
                    /*Codes_SRS_SASTOKEN_25_029: [**SASToken_validate shall check for expiry time from token and if token has expired then would return false **]***/
                    if (expiry <= 0)
                    {
                        result = false;
                    }
                    else
                    {
                        double secSinceEpoch = get_difftime(get_time(NULL), (time_t)0);
                        if (expiry < secSinceEpoch)
                        {
                            /*Codes_SRS_SASTOKEN_25_029: [**SASToken_validate shall check for expiry time from token and if token has expired then would return false **]***/
                            result = false;
                        }
                        else
                        {
                            /*Codes_SRS_SASTOKEN_25_030: [**SASToken_validate shall return true only if the format is obeyed and the token has not yet expired **]***/
                            result = true;
                        }
                    }
                    free(expiryASCII);
                }
            }
        }
    }

    return result;
}

STRING_HANDLE SASToken_Create(STRING_HANDLE key, STRING_HANDLE scope, STRING_HANDLE keyName, size_t expiry)
{
    STRING_HANDLE result = NULL;
    char tokenExpirationTime[32] = { 0 };

    /*Codes_SRS_SASTOKEN_06_001: [If key is NULL then SASToken_Create shall return NULL.]*/
    /*Codes_SRS_SASTOKEN_06_003: [If scope is NULL then SASToken_Create shall return NULL.]*/
    /*Codes_SRS_SASTOKEN_06_007: [If keyName is NULL then SASToken_Create shall return NULL.]*/
    if ((key == NULL) ||
        (scope == NULL) ||
        (keyName == NULL))
    {
        LogError("Invalid Parameter to SASToken_Create. handle key: %p, handle scope: %p, handle keyName: %p", key, scope, keyName);
    }
    else
    {
        BUFFER_HANDLE decodedKey;
        /*Codes_SRS_SASTOKEN_06_029: [The key parameter is decoded from base64.]*/
        if ((decodedKey = Base64_Decoder(STRING_c_str(key))) == NULL)
        {
            /*Codes_SRS_SASTOKEN_06_030: [If there is an error in the decoding then SASToken_Create shall return NULL.]*/
            LogError("Unable to decode the key for generating the SAS.");
        }
        else
        {
            /*Codes_SRS_SASTOKEN_06_026: [If the conversion to string form fails for any reason then SASToken_Create shall return NULL.]*/
            if (size_tToString(tokenExpirationTime, sizeof(tokenExpirationTime), expiry) != 0)
            {
                LogError("For some reason converting seconds to a string failed.  No SAS can be generated.");
            }
            else
            {
                STRING_HANDLE toBeHashed = NULL;
                BUFFER_HANDLE hash = NULL;
                if (((hash = BUFFER_new()) == NULL) ||
                    ((toBeHashed = STRING_new()) == NULL) ||
                    ((result = STRING_new()) == NULL))
                {
                    LogError("Unable to allocate memory to prepare SAS token.");
                }
                else
                {
                    /*Codes_SRS_SASTOKEN_06_009: [The scope is the basis for creating a STRING_HANDLE.]*/
                    /*Codes_SRS_SASTOKEN_06_010: [A "\n" is appended to that string.]*/
                    /*Codes_SRS_SASTOKEN_06_011: [tokenExpirationTime is appended to that string.]*/
                    if ((STRING_concat_with_STRING(toBeHashed, scope) != 0) ||
                        (STRING_concat(toBeHashed, "\n") != 0) ||
                        (STRING_concat(toBeHashed, tokenExpirationTime) != 0))
                    {
                        LogError("Unable to build the input to the HMAC to prepare SAS token.");
                        STRING_delete(result);
                        result = NULL;
                    }
                    else
                    {
                        STRING_HANDLE base64Signature = NULL;
                        STRING_HANDLE urlEncodedSignature = NULL;
                        size_t inLen = STRING_length(toBeHashed);
                        const unsigned char* inBuf = (const unsigned char*)STRING_c_str(toBeHashed);
                        size_t outLen = BUFFER_length(decodedKey);
                        unsigned char* outBuf = BUFFER_u_char(decodedKey);
                        /*Codes_SRS_SASTOKEN_06_013: [If an error is returned from the HMAC256 function then NULL is returned from SASToken_Create.]*/
                        /*Codes_SRS_SASTOKEN_06_012: [An HMAC256 hash is calculated using the decodedKey, over toBeHashed.]*/
                        /*Codes_SRS_SASTOKEN_06_014: [If there are any errors from the following operations then NULL shall be returned.]*/
                        /*Codes_SRS_SASTOKEN_06_015: [The hash is base 64 encoded.]*/
                        /*Codes_SRS_SASTOKEN_06_028: [base64Signature shall be url encoded.]*/
                        /*Codes_SRS_SASTOKEN_06_016: [The string "SharedAccessSignature sr=" is the first part of the result of SASToken_Create.]*/
                        /*Codes_SRS_SASTOKEN_06_017: [The scope parameter is appended to result.]*/
                        /*Codes_SRS_SASTOKEN_06_018: [The string "&sig=" is appended to result.]*/
                        /*Codes_SRS_SASTOKEN_06_019: [The string urlEncodedSignature shall be appended to result.]*/
                        /*Codes_SRS_SASTOKEN_06_020: [The string "&se=" shall be appended to result.]*/
                        /*Codes_SRS_SASTOKEN_06_021: [tokenExpirationTime is appended to result.]*/
                        /*Codes_SRS_SASTOKEN_06_022: [The string "&skn=" is appended to result.]*/
                        /*Codes_SRS_SASTOKEN_06_023: [The argument keyName is appended to result.]*/
                        if ((HMACSHA256_ComputeHash(outBuf, outLen, inBuf, inLen, hash) != HMACSHA256_OK) ||
                            ((base64Signature = Base64_Encode(hash)) == NULL) ||
                            ((urlEncodedSignature = URL_Encode(base64Signature)) == NULL) ||
                            (STRING_copy(result, "SharedAccessSignature sr=") != 0) ||
                            (STRING_concat_with_STRING(result, scope) != 0) ||
                            (STRING_concat(result, "&sig=") != 0) ||
                            (STRING_concat_with_STRING(result, urlEncodedSignature) != 0) ||
                            (STRING_concat(result, "&se=") != 0) ||
                            (STRING_concat(result, tokenExpirationTime) != 0) ||
                            (STRING_concat(result, "&skn=") != 0) ||
                            (STRING_concat_with_STRING(result, keyName) != 0))
                        {
                            LogError("Unable to build the SAS token.");
                            STRING_delete(result);
                            result = NULL;
                        }
                        else
                        {
                            /* everything OK */
                        }
                        STRING_delete(base64Signature);
                        STRING_delete(urlEncodedSignature);
                    }
                }
                STRING_delete(toBeHashed);
                BUFFER_delete(hash);
            }
            BUFFER_delete(decodedKey);
        }
    }

    return result;
}

