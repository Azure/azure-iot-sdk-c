// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#include "jsondecoder.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

#define IsWhiteSpace(A) (((A) == 0x20) || ((A) == 0x09) || ((A) == 0x0A) || ((A) == 0x0D))

typedef struct PARSER_STATE_TAG
{
    char* json;
} PARSER_STATE;

static JSON_DECODER_RESULT ParseArray(PARSER_STATE* parserState, MULTITREE_HANDLE currentNode);
static JSON_DECODER_RESULT ParseObject(PARSER_STATE* parserState, MULTITREE_HANDLE currentNode);

static void NoFreeFunction(void* value)
{
    (void)value;
}

static int NOPCloneFunction(void** destination, const void* source)
{
    *destination = (void**)source;
    return 0;
}

void SkipWhiteSpaces(PARSER_STATE* parserState)
{
    while ((*(parserState->json) != '\0') && IsWhiteSpace(*(parserState->json)))
    {
        parserState->json++;
    }
}

static JSON_DECODER_RESULT ParseString(PARSER_STATE* parserState, char** stringBegin)
{
    JSON_DECODER_RESULT result = JSON_DECODER_OK;
    *stringBegin = parserState->json;

    if (*(parserState->json) != '"')
    {
        result = JSON_DECODER_PARSE_ERROR;
    }
    else
    {
        parserState->json++;
        while ((*(parserState->json) != '"') && (*(parserState->json) != '\0'))
        {
            if (*(parserState->json) == '\\')
            {
                parserState->json++;
                if (
                    (*parserState->json == '\\') ||
                    (*parserState->json == '"') ||
                    (*parserState->json == '/') ||
                    (*parserState->json == 'b') ||
                    (*parserState->json == 'f') ||
                    (*parserState->json == 'n') ||
                    (*parserState->json == 'r') ||
                    (*parserState->json == 't'))
                {
                    parserState->json++;
                }
                else
                {
                    result = JSON_DECODER_PARSE_ERROR;
                    break;
                }
            }
            else
            {
                parserState->json++;
            }
        }

        if (result == JSON_DECODER_OK)
        {
            if (*(parserState->json) != '"')
            {
                result = JSON_DECODER_PARSE_ERROR;
            }
            else
            {
                parserState->json++;
                result = JSON_DECODER_OK;
            }
        }
    }

    return result;
}

static JSON_DECODER_RESULT ParseNumber(PARSER_STATE* parserState)
{
    JSON_DECODER_RESULT result = JSON_DECODER_OK;
    size_t digitCount = 0;

    if (*(parserState->json) == '-')
    {
        parserState->json++;
    }

    while (*(parserState->json) != '\0')
    {
        if (ISDIGIT(*(parserState->json)))
        {
            digitCount++;
            /* simply continue */
        }
        else
        {
            break;
        }

        parserState->json++;
    }

    if ((digitCount == 0) ||
        ((digitCount > 1) && *(parserState->json - digitCount) == '0'))
    {
        result = JSON_DECODER_PARSE_ERROR;
    }
    else
    {
        if (*(parserState->json) == '.')
        {
            /* optional fractional part */
            parserState->json++;
            digitCount = 0;

            while (*(parserState->json) != '\0')
            {
                if (ISDIGIT(*(parserState->json)))
                {
                    digitCount++;
                    /* simply continue */
                }
                else
                {
                    break;
                }

                parserState->json++;
            }

            if (digitCount == 0)
            {
                result = JSON_DECODER_PARSE_ERROR;
            }
        }

        if ((*(parserState->json) == 'e') || (*(parserState->json) == 'E'))
        {
            parserState->json++;

            /* optional sign */
            if ((*(parserState->json) == '-') || (*(parserState->json) == '+'))
            {
                parserState->json++;
            }

            digitCount = 0;

            while (*(parserState->json) != '\0')
            {
                if (ISDIGIT(*(parserState->json)))
                {
                    digitCount++;
                    /* simply continue */
                }
                else
                {
                    break;
                }

                parserState->json++;
            }

            if (digitCount == 0)
            {
                result = JSON_DECODER_PARSE_ERROR;
            }
        }
    }

    return result;
}

static JSON_DECODER_RESULT ParseValue(PARSER_STATE* parserState, MULTITREE_HANDLE currentNode, char** stringBegin)
{
    JSON_DECODER_RESULT result;

    SkipWhiteSpaces(parserState);

    if (*(parserState->json) == '"')
    {
        result = ParseString(parserState, stringBegin);
    }
    else if (strncmp(parserState->json, "false", 5) == 0)
    {
        *stringBegin = parserState->json;
        parserState->json += 5;
        result = JSON_DECODER_OK;
    }
    else if (strncmp(parserState->json, "true", 4) == 0)
    {
        *stringBegin = parserState->json;
        parserState->json += 4;
        result = JSON_DECODER_OK;
    }
    else if (strncmp(parserState->json, "null", 4) == 0)
    {
        *stringBegin = parserState->json;
        parserState->json += 4;
        result = JSON_DECODER_OK;
    }
    else if (*(parserState->json) == '[')
    {
        result = ParseArray(parserState, currentNode);
        *stringBegin = NULL;
    }
    else if (*(parserState->json) == '{')
    {
        result = ParseObject(parserState, currentNode);
        *stringBegin = NULL;
    }
    else if (
        (
            ISDIGIT(*(parserState->json))
        )
        || (*(parserState->json) == '-'))
    {
        *stringBegin = parserState->json;
        result = ParseNumber(parserState);
    }
    else
    {
        result = JSON_DECODER_PARSE_ERROR;
    }

    return result;
}

static JSON_DECODER_RESULT ParseColon(PARSER_STATE* parserState)
{
    JSON_DECODER_RESULT result;

    SkipWhiteSpaces(parserState);
    if (*(parserState->json) != ':')
    {
        result = JSON_DECODER_PARSE_ERROR;
    }
    else
    {
        parserState->json++;
        result = JSON_DECODER_OK;
    }

    return result;
}

static JSON_DECODER_RESULT ParseOpenCurly(PARSER_STATE* parserState)
{
    JSON_DECODER_RESULT result = JSON_DECODER_OK;

    SkipWhiteSpaces(parserState);

    if (*(parserState->json) != '{')
    {
        result = JSON_DECODER_PARSE_ERROR;
    }
    else
    {
        parserState->json++;
        result = JSON_DECODER_OK;
    }

    return result;
}

static JSON_DECODER_RESULT ParseNameValuePair(PARSER_STATE* parserState, MULTITREE_HANDLE currentNode)
{
    JSON_DECODER_RESULT result;
    char* memberNameBegin;

    SkipWhiteSpaces(parserState);

    result = ParseString(parserState, &memberNameBegin);
    if (result == JSON_DECODER_OK)
    {
        char* valueBegin;
        MULTITREE_HANDLE childNode;
        *(parserState->json - 1) = 0;

        result = ParseColon(parserState);
        if (result == JSON_DECODER_OK)
        {
            /* Multi Tree takes care of not having 2 children with the same name */
            if (MultiTree_AddChild(currentNode, memberNameBegin + 1, &childNode) != MULTITREE_OK)
            {
                result = JSON_DECODER_MULTITREE_FAILED;
            }
            else
            {
                result = ParseValue(parserState, childNode, &valueBegin);
                if ((result == JSON_DECODER_OK) && (valueBegin != NULL))
                {
                    if (MultiTree_SetValue(childNode, valueBegin) != MULTITREE_OK)
                    {
                        result = JSON_DECODER_MULTITREE_FAILED;
                    }
                }
            }
        }
    }

    return result;
}

static JSON_DECODER_RESULT ParseObject(PARSER_STATE* parserState, MULTITREE_HANDLE currentNode)
{
    JSON_DECODER_RESULT result = ParseOpenCurly(parserState);
    if (result == JSON_DECODER_OK)
    {
        char jsonChar;

        SkipWhiteSpaces(parserState);

        jsonChar = *(parserState->json);
        while ((jsonChar != '}') && (jsonChar != '\0'))
        {
            char* valueEnd;

            /* decode each value */
            result = ParseNameValuePair(parserState, currentNode);
            if (result != JSON_DECODER_OK)
            {
                break;
            }

            valueEnd = parserState->json;

            SkipWhiteSpaces(parserState);
            jsonChar = *(parserState->json);
            *valueEnd = 0;

            if (jsonChar == ',')
            {
                parserState->json++;
                /* get the next name/value pair */
            }
        }

        if (result != JSON_DECODER_OK)
        {
            /* already have error */
        }
        else
        {
            if (jsonChar != '}')
            {
                result = JSON_DECODER_PARSE_ERROR;
            }
            else
            {
                parserState->json++;
            }
        }
    }

    return result;
}

static JSON_DECODER_RESULT ParseArray(PARSER_STATE* parserState, MULTITREE_HANDLE currentNode)
{
    JSON_DECODER_RESULT result = JSON_DECODER_OK;

    SkipWhiteSpaces(parserState);

    if (*(parserState->json) != '[')
    {
        result = JSON_DECODER_PARSE_ERROR;
    }
    else
    {
        char* stringBegin;
        char jsonChar;
        int arrayIndex = 0;
        result = JSON_DECODER_OK;

        parserState->json++;

        SkipWhiteSpaces(parserState);

        jsonChar = *parserState->json;
        while ((jsonChar != ']') && (jsonChar != '\0'))
        {
            char arrayIndexStr[22];
            MULTITREE_HANDLE childNode;

            if (sprintf(arrayIndexStr, "%d", arrayIndex++) < 0)
            {
                result = JSON_DECODER_ERROR;
                break;
            }
            else if (MultiTree_AddChild(currentNode, arrayIndexStr, &childNode) != MULTITREE_OK)
            {
                result = JSON_DECODER_MULTITREE_FAILED;
            }
            else
            {
                char* valueEnd;

                /* decode each value */
                result = ParseValue(parserState, childNode, &stringBegin);
                if (result != JSON_DECODER_OK)
                {
                    break;
                }

                if (stringBegin != NULL)
                {
                    if (MultiTree_SetValue(childNode, stringBegin) != MULTITREE_OK)
                    {
                        result = JSON_DECODER_MULTITREE_FAILED;
                        break;
                    }
                }

                valueEnd = parserState->json;

                SkipWhiteSpaces(parserState);
                jsonChar = *(parserState->json);
                *valueEnd = 0;

                if (jsonChar == ',')
                {
                    parserState->json++;
                    /* get the next value pair */
                }
                else if (jsonChar == ']')
                {
                    break;
                }
                else
                {
                    result = JSON_DECODER_PARSE_ERROR;
                    break;
                }
            }
        }

        if (result != JSON_DECODER_OK)
        {
            /* already have error */
        }
        else
        {
            if (jsonChar != ']')
            {
                result = JSON_DECODER_PARSE_ERROR;
            }
            else
            {
                parserState->json++;
                SkipWhiteSpaces(parserState);
            }
        }
    }

    return result;
}

static JSON_DECODER_RESULT ParseObjectOrArray(PARSER_STATE* parserState, MULTITREE_HANDLE currentNode)
{
    JSON_DECODER_RESULT result = JSON_DECODER_PARSE_ERROR;

    SkipWhiteSpaces(parserState);

    if (*(parserState->json) == '{')
    {
        result = ParseObject(parserState, currentNode);
        SkipWhiteSpaces(parserState);
    }
    else if (*(parserState->json) == '[')
    {
        result = ParseArray(parserState, currentNode);
        SkipWhiteSpaces(parserState);
    }
    else
    {
        result = JSON_DECODER_PARSE_ERROR;
    }

    if ((result == JSON_DECODER_OK) &&
        (*(parserState->json) != '\0'))
    {
        result = JSON_DECODER_PARSE_ERROR;
    }

    return result;
}

static JSON_DECODER_RESULT ParseJSON(char* json, MULTITREE_HANDLE currentNode)
{
    PARSER_STATE parseState;
    parseState.json = json;
    return ParseObjectOrArray(&parseState, currentNode);
}

JSON_DECODER_RESULT JSONDecoder_JSON_To_MultiTree(char* json, MULTITREE_HANDLE* multiTreeHandle)
{
    JSON_DECODER_RESULT result;

    if ((json == NULL) ||
        (multiTreeHandle == NULL))
    {
        result = JSON_DECODER_INVALID_ARG;
    }
    else if (*json == '\0')
    {
        result = JSON_DECODER_PARSE_ERROR;
    }
    else
    {
        *multiTreeHandle = MultiTree_Create(NOPCloneFunction, NoFreeFunction);
        if (*multiTreeHandle == NULL)
        {
            result = JSON_DECODER_MULTITREE_FAILED;
        }
        else
        {
            result = ParseJSON(json, *multiTreeHandle);
            if (result != JSON_DECODER_OK)
            {
                MultiTree_Destroy(*multiTreeHandle);
            }
        }
    }

    return result;
}
