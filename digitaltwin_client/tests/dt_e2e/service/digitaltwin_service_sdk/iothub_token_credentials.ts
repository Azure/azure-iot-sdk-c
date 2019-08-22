/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for
 * license information.
 */

import { ServiceClientCredentials, WebResource, Constants } from '@azure/ms-rest-js';
import { ConnectionString, SharedAccessSignature, anHourFromNow } from 'azure-iot-common';

export class IoTHubTokenCredentials implements ServiceClientCredentials {
  private _connectionString: ConnectionString;

  constructor(connectionString: string) {
    this._connectionString = ConnectionString.parse(connectionString, ['HostName', 'SharedAccessKeyName', 'SharedAccessKey']);
  }

  signRequest(webResource: WebResource): Promise<WebResource> {
    const sas = SharedAccessSignature.create(this._connectionString.HostName as string, this._connectionString.SharedAccessKeyName as string, this._connectionString.SharedAccessKey as string, anHourFromNow()).toString();
    webResource.headers.set(Constants.HeaderConstants.AUTHORIZATION, sas);
    return Promise.resolve(webResource);
  }
}