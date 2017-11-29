# Managing CA Certificates Sample

## WARNING
Certificates created by these scripts **MUST NOT** be used for production.  They contain hard-coded passwords ("123"), expire after 30 days, and most importantly are provided for demonstration purposes to help you quickly understand CA Certificates.  When productizing against CA Certificates, you'll need to use your own security best practices for certification creation and lifetime management.

## Introduction
This document assumes you understand the core scenarios and motivation behind CA Certificates.  This document also assumes you have basic familiarity with PowerShell or Bash.

This directory contains a PowerShell (PS1) and Bash script to help create **test** certificates for Azure IoT Hub's CA Certificate and proof-of-possession.  They will create a Root CA, intermediate, leaf signed up to CA or intermediate, and help with the proof-of-possession flow.

The PS1 and Bash scripts are functionally equivalent; they are both provided depending on your preference for Windows or Linux.

A more detailed document showing UI screen shots is available from [the official documentation].

## USE

## Step 1 - Initial Setup
You'll need to do some initial setup prior to running these scripts.

###  **PowerShell**
* Start PowerShell as an Administrator.
* `cd` to the directory you want to run in.  All files will be created as children of this directory.
* Run `Set-ExecutionPolicy -ExecutionPolicy Unrestricted`.  You'll need this for PowerShell to allow you to run the scripts.
* Get OpenSSL for Windows.  
  * See [the official documentation] for places to download it.
  * Set `$ENV:OPENSSL_CONF` to the openssl's openssl.cnf.
* Run `. .\ca-certs.ps1` .  This is called dot-sourcing and brings the functions of the script into PowerShell's global namespace.
* Run `Test-CACertsPrerequisites`.
 PowerShell uses the Windows Certificate Store to manage certificates.  This makes sure that there won't be name collisions later with existing certificates and that OpenSSL is setup correctly.

###  **Bash**
* Start Bash.
* `cd` to the directory you want to run in.  All files will be created as children of this directory.
* `cp *.cnf` and `cp *.sh` from the directory this .MD file is located into your working directory.
* `chmod 700 certGen.sh` 


## Step 2 - Create the certificate chain
First you need to create a CA and an intermediate certificate signer that chains back to the CA.

### **PowerShell**
* Run `New-CACertsCertChain`.  Note this updates your Windows Certificate store with these certs.

### **Bash**
* Run `./certGen.sh create_root_and_intermediate`

Next, go to Azure IoT Hub and navigate to Certificates.  Add a new certificate, providing the root CA file when prompted.  (`.\RootCA.pem` in PowerShell and `./certs/azure-iot-test-only.root.ca.cert.pem` in Bash.)

## Step 3 - Proof of Possession
Now that you've registered your root CA with Azure IoT Hub, you'll need to prove that you actually own it.

Select the new certificate that you've created and navigate to and select  "Generate Verification Code".  This will give you a string that specifies the subject name of a certificate that you need to sign.  For our example, assume IoT Hub wants you to create a certificate with subject name = "12345".

### **PowerShell**
* Run  `New-CACertsVerificationCert "12345"`

### **Bash**
* Run `./certGen.sh create_verification_certificate 12345`

In both cases, the scripts will output the name of the file containing `"CN=12345"` to the console.  Upload this file to IoT Hub (in the same UX that had the "Generate Verification Code") and select "Verify".

## Step 4 - Create a new device
Finally, let's create an application and corresponding device on IoT Hub that shows how CA Certificates are used.

On Azure IoT Hub, navigate to the "Device Explorer".  Add a new device (e.g. `myDevice`, and for its authentication type chose "X.509 CA Signed".  Devices can authenticate to IoT Hub using a certificate that is signed by the Root CA from Step 2.

### **PowerShell**
* Run `New-CACertsDevice myDevice` to create the new device certificate.  
This will create files myDevice* that contain the public key, private key, and PFX of this certificate.  When prompted to enter a password during the signing process, enter "123".

* To get a sense of how to use these certificates, `Write-CACertsCertificatesToEnvironment myDevice myIotHubName`, replacing myDevice and myIotHub name with your values.  This will create the environment variables `$ENV:IOTHUB_CA_*` that can give a sense of how they could be consumed by an application.

### **Bash**
* Run `./certGen.sh create_device_certificate myDevice` to create the new device certificate.  
  This will create the files .\certs\new-device.* that contain the public key and PFX and .\private\new-device.key.pem that contains the device's private key.  
* `cat new-device.cert.pem azure-iot-test-only.intermediate.cert.pem azure-iot-test-only.root.ca.cert.pem > new-device-full-chain.cert.pem` to get the public key.
* `./private/new-device.cert.pem` contains the device's private key.


## Step 5 - Cleanup
For PowerShell, open `manage computer certificates` and navigate Certificates -Local Compturer-->personal.  Remove certificates issued by "Azure IoT CA TestOnly*".  Similarly remove them from "Trusted Root Certification Authority->Certificates" and "Intermediate Certificate Authorities->Certificates".

Bash outputs certificates to the current working directory, so there is no analogous system cleanup needed.

[the official documentation]: https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-security-x509-get-started
