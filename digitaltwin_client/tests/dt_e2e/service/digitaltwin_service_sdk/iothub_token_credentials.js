"use strict";
/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for
 * license information.
 */
exports.__esModule = true;
var ms_rest_js_1 = require("@azure/ms-rest-js");
var azure_iot_common_1 = require("azure-iot-common");
var IoTHubTokenCredentials = /** @class */ (function () {
    function IoTHubTokenCredentials(connectionString) {
        this._connectionString = azure_iot_common_1.ConnectionString.parse(connectionString, ['HostName', 'SharedAccessKeyName', 'SharedAccessKey']);
    }
    IoTHubTokenCredentials.prototype.signRequest = function (webResource) {
        var sas = azure_iot_common_1.SharedAccessSignature.create(this._connectionString.HostName, this._connectionString.SharedAccessKeyName, this._connectionString.SharedAccessKey, azure_iot_common_1.anHourFromNow()).toString();
        webResource.headers.set(ms_rest_js_1.Constants.HeaderConstants.AUTHORIZATION, sas);
        return Promise.resolve(webResource);
    };
    return IoTHubTokenCredentials;
}());
exports.IoTHubTokenCredentials = IoTHubTokenCredentials;
