#!/bin/bash

ModuleUser=$(printenv ModuleUser)
EdgeModuleCACertificateFile=$(printenv EdgeModuleCACertificateFile)

echo "Edge Chain CA Certificate File: ${EdgeModuleCACertificateFile}"

# copy the CA cert into the ca cert dir
command="cp ${EdgeModuleCACertificateFile} /usr/local/share/ca-certificates/edge-chain-ca.crt"
echo "Executing: ${command}"
${command}
if [ $? -ne 0 ]; then
    echo "Failed to Copy Edge Chain CA Certificate."
    exit 1
fi
# register the newly added CA cert
command="update-ca-certificates"
echo "Executing: ${command}"
${command}
if [ $? -ne 0 ]; then
    echo "Failed to Update CA Certificates."
    exit 1
fi
echo "Certificates installed successfully!"

# start service
runuser -u "$ModuleUser" /app/iothub_client_sample_module_sender
