// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MACROE2EMODELACTION_H
#define MACROE2EMODELACTION_H

#include "serializer_devicetwin.h"

#ifdef __cplusplus
extern "C" {
#endif

BEGIN_NAMESPACE(MacroE2EModelAction)

DECLARE_DEVICETWIN_MODEL(deviceModel,
    WITH_DATA(ascii_char_ptr, property1),
    WITH_DATA(int, UniqueId),
    WITH_ACTION(dataMacroCallback, ascii_char_ptr, property1, int, UniqueId),
    WITH_METHOD(theSumOfThings, int, a, int, b),
    WITH_REPORTED_PROPERTY(int, reported_int)
)

END_NAMESPACE(MacroE2EModelAction);

#ifdef __cplusplus
}
#endif

#endif /* MACROE2EMODELACTION_H */
