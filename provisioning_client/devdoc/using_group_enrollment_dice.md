# Group Enrollment using DICE emulator

This document will detail the steps to create a group enrollment in the azure portal using the IotHub C SDK.  This document will make the following assumptions:

- The IoTHub C SDK is downloaded and built with Provisioning Enabled

- You have access to the Azure Portal

## DICE device enrollment Tool

The C SDK provides a Tool to help with the creation of enrollment group certificates.  Navigate to the dice_device_enrollment application in the cmake directory:

```Shell
$/azure-iot-sdk-c/cmake/provisioning_client/tools/dice_device_enrollment/dice_device_enrollment
```

Run the tool and at the prompt enter 'g'

```Shell
`Would you like to do Individual (i) or Group (g) enrollments: `
```

This will print out the signer certificate that will need to be saved as a PEM certificate file.  Save the certificate as signer_cert.pem.

Go to the azure portal provisioning page and create a new Enrollment Group using the previously saved signer_cert.pem file.  Once the enrollment is saved, got to the Certificates tab and select the Unverified `signer_cert`.  Once selected you will need to click the `Generate Verification Code` button in the Certificate Details pane.  

Copy the Verification Code back into the command prompt at the prompt

```Shell
`Enter the Validation Code (Press enter when finished):`
```

This will print out the verification certificate that you will need to save (in PEM format as above) and select back in the Certificate Detail pane in the portal.

This should verify the certificate and upon refreshing will show verified.