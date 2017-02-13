// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SINGLYLINKEDLIST_H
#define SINGLYLINKEDLIST_H

#ifdef __cplusplus
extern "C" {
#include <cstdbool>
#else
#include "stdbool.h"
#endif /* __cplusplus */

#include "umock_c_prod.h"

typedef struct SINGLYLINKEDLIST_INSTANCE_TAG* SINGLYLINKEDLIST_HANDLE;
typedef struct LIST_ITEM_INSTANCE_TAG* LIST_ITEM_HANDLE;
typedef bool (*LIST_MATCH_FUNCTION)(LIST_ITEM_HANDLE list_item, const void* match_context);

MOCKABLE_FUNCTION(, SINGLYLINKEDLIST_HANDLE, singlylinkedlist_create);
MOCKABLE_FUNCTION(, void, singlylinkedlist_destroy, SINGLYLINKEDLIST_HANDLE, list);
MOCKABLE_FUNCTION(, LIST_ITEM_HANDLE, singlylinkedlist_add, SINGLYLINKEDLIST_HANDLE, list, const void*, item);
MOCKABLE_FUNCTION(, int, singlylinkedlist_remove, SINGLYLINKEDLIST_HANDLE, list, LIST_ITEM_HANDLE, item_handle);
MOCKABLE_FUNCTION(, LIST_ITEM_HANDLE, singlylinkedlist_get_head_item, SINGLYLINKEDLIST_HANDLE, list);
MOCKABLE_FUNCTION(, LIST_ITEM_HANDLE, singlylinkedlist_get_next_item, LIST_ITEM_HANDLE, item_handle);
MOCKABLE_FUNCTION(, LIST_ITEM_HANDLE, singlylinkedlist_find, SINGLYLINKEDLIST_HANDLE, list, LIST_MATCH_FUNCTION, match_function, const void*, match_context);
MOCKABLE_FUNCTION(, const void*, singlylinkedlist_item_get_value, LIST_ITEM_HANDLE, item_handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SINGLYLINKEDLIST_H */
