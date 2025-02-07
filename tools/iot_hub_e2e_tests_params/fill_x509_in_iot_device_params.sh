if ! command -v sed 2>&1 >/dev/null ; then echo "Please install sed"; exit 1; fi 

rm ./cert.pem 2>&1 >/dev/null
rm ./key.pem 2>&1 >/dev/null

openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -nodes -days 365 -subj "/C=US/ST=Washington/L=Redmond/O=Company/OU=Org/CN=www.company.com"
sed -i "s/IOTHUB_E2E_X509_CERT_BASE64='.*'/IOTHUB_E2E_X509_CERT_BASE64='$(base64 -w 0 ./cert.pem)'/g" ./iot_device_params.txt
sed -i "s/IOTHUB_E2E_X509_PRIVATE_KEY_BASE64='.*'/IOTHUB_E2E_X509_PRIVATE_KEY_BASE64='$(base64 -w 0 ./key.pem)'/g" ./iot_device_params.txt
sed -i "s/IOTHUB_E2E_X509_THUMBPRINT='.*'/IOTHUB_E2E_X509_THUMBPRINT='$(openssl x509 -noout -fingerprint -inform pem -in cert.pem | sed 's/.*=//g' | sed 's/://g')'/g" ./iot_device_params.txt
