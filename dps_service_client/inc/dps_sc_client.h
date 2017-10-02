// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DPS_SC_CLIENT_H
#define DPS_SC_CLIENT_H

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#include "dps_sc_enrollment.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    #define BULK_OPERATION_MODE_VALUES \
        BULK_CREATE, \
        BULK_UPDATE, \
        BULK_UPDATE_IF_MATCH_ETAG, \
        BULK_DELETE

    DEFINE_ENUM(BULK_OPERATION_MODE, BULK_OPERATION_MODE_VALUES);

    /** @brief  Handle to hide struct and use it in consequent APIs
    */
    typedef struct DPS_SC_TAG* DPS_SC_HANDLE;

    /** @brief  Creates a DPS_SC handle for use in consequent APIs.
    *
    * @param    conn_string     A connection string used to establish connection with DPS_SC.
    *
    * @return   A non-NULL DPS_SC_HANDLE value that is used when invoking other functins in the DPS_SC
    *           and NULL on failure.
    */
    MOCKABLE_FUNCTION(, DPS_SC_HANDLE, dps_sc_create_from_connection_string, const char*, conn_string);
    
    /** @brief  Disposes of resources allocated by creating a DPS_SC handle.
    *
    * @param    handle      The handle created by a call to the create function.
    */
    MOCKABLE_FUNCTION(, void, dps_sc_destroy, DPS_SC_HANDLE, handle);

    /** @brief Creates or updates a device enrollment record on the DPS_SC.
    *
    * @param    handle      The handle used for connecting to the DPS_SC.
    * @param    id          The registration id of the target enrollment.
    * @param    enrollment  A struct describing the desired changes to the enrollment.
    *
    * @return   0 upon success, a non-zero number upon failure.
    */
    MOCKABLE_FUNCTION(, int, dps_sc_create_or_update_enrollment, DPS_SC_HANDLE, handle, const char*, id, const ENROLLMENT*, enrollment);
    
    /** @brief  Deletes a device enrollment record on the DPS_SC.
    *
    * @param    handle      The handle used for connecting to the DPS_SC.
    * @param    id          The registration id of the target enrollment.
    *
    * @return   0 upon success, a non-zero number upon failure.
    */
    MOCKABLE_FUNCTION(, int, dps_sc_delete_enrollment, DPS_SC_HANDLE, handle, const char*, id);

    /** @breif  Retreives a device enrollment record from the DPS_SC.
    *
    * @param    handle      The handle used for connecting to the DPS_SC.
    * @param    id          The registration id of the target enrollment.
    * @param    enrollment  A struct representing an enrollment, to be filled with retreived data.
    *
    * @return   0 upon success, a non-zero number upon failure.
    */
    MOCKABLE_FUNCTION(, int, dps_sc_get_enrollment, DPS_SC_HANDLE, handle, const char*, id, ENROLLMENT*, enrollment);

    /** @brief  Deletes a device registration status on the DPS_SC.
    *
    * @param    handle      The handle used for connecting to the DPS_SC.
    * @param    id          The registration id of the target enrollment.
    *
    * @return   0 upon success, a non-zero number upon failure.
    */
    MOCKABLE_FUNCTION(, int, dps_sc_delete_device_registration_status, DPS_SC_HANDLE, handle, const char*, id);

    /** @brief  Retreives a device registration status from the DPS_SC.
    *
    * @param    handle      A handle used for connecting to the DPS_SC.
    * @param    id          The registration id of the target registration status.
    * @param    reg_status  A struct representing a registration status, to be filled with retreived data.
    *
    * @return   0 upon success, a non-zero number upon failure.
    */
    MOCKABLE_FUNCTION(, int, dps_sc_get_device_registration_status, DPS_SC_HANDLE, handle, const char*, id, DEVICE_REGISTRATION_STATUS*, reg_status);

    /** @brief  Creates or updates a device enrollment group record on the DPS_SC.
    *
    * @param    handle              The handle used for connecting to the DPS_SC.
    * @param    id                  The enrollment group id of the target enrollment group.
    * @param    enrollment_group    A struct describing the desired changes to the enrollment group.
    *
    * @return   0 upon success, a non-zero number upon failure.
    */
    MOCKABLE_FUNCTION(, int, dps_sc_create_or_update_enrollment_group, DPS_SC_HANDLE, handle, const char*, id, const ENROLLMENT_GROUP*, enrollment_group);

    /** @brief  Deletes a device enrollment group record on the DPS_SC.
    * @param    handle      The handle used for connecting to the DPS_SC.
    * @param    id          The enrollment group id of the target enrollment group.
    *
    * @return   0 upon success, a non-zero number upon failure.
    */
    MOCKABLE_FUNCTION(, int, dps_sc_delete_enrollment_group, DPS_SC_HANDLE, handle, const char*, id);

    /** @brief  Retreives a device enrollment group record from the DPS_SC.
    *
    * @param    handle              The handle used for connecting to the DPS_SC.
    * @param    id                  The enrollment group id of the target enrollment group.
    * @param    enrollment_group    A struct representing an enrollment group, to be filled with the retreived data.
    *
    * @return   0 upon success, a non-zero number upon failure.
    */
    MOCKABLE_FUNCTION(, int, dps_sc_get_enrollment_group, DPS_SC_HANDLE, handle, const char*, id, ENROLLMENT_GROUP*, enrollment_group);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DPS_SC_CLIENT_H */
