// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_shared_utility/gballoc.h"

#include "dataserializer.h"
#include "azure_c_shared_utility/xlogging.h"

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(DATA_SERIALIZER_RESULT, DATA_SERIALIZER_RESULT_VALUES);

BUFFER_HANDLE DataSerializer_Encode(MULTITREE_HANDLE multiTreeHandle, DATA_SERIALIZER_MULTITREE_TYPE dataType, DATA_SERIALIZER_ENCODE_FUNC encodeFunc)
{
    BUFFER_HANDLE pBuffer;

    if (multiTreeHandle == NULL || encodeFunc == NULL)
    {
        pBuffer = NULL;
        LogError("(Error code: %s)", MU_ENUM_TO_STRING(DATA_SERIALIZER_RESULT, DATA_SERIALIZER_INVALID_ARG) );
    }
    else
    {
        pBuffer = encodeFunc(multiTreeHandle, dataType);
        if (pBuffer == NULL)
        {
            LogError("(Error code: %s)", MU_ENUM_TO_STRING(DATA_SERIALIZER_RESULT, DATA_SERIALIZER_ERROR) );
        }
    }
    return pBuffer;
}

MULTITREE_HANDLE DataSerializer_Decode(BUFFER_HANDLE data, DATA_SERIALIZER_DECODE_FUNC decodeFunc)
{
    MULTITREE_HANDLE multiTreeHandle;

    if (data == NULL || decodeFunc == NULL)
    {
        multiTreeHandle = NULL;
        LogError("(Error code: %s)", MU_ENUM_TO_STRING(DATA_SERIALIZER_RESULT, DATA_SERIALIZER_INVALID_ARG) );
    }
    else
    {
        multiTreeHandle = decodeFunc(data);
        if (multiTreeHandle == NULL)
        {
            LogError("(Error code: %s)", MU_ENUM_TO_STRING(DATA_SERIALIZER_RESULT, DATA_SERIALIZER_ERROR) );
        }
    }

    return multiTreeHandle;
}
