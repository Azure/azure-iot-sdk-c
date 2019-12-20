echo $NEWTIN | sudo -S rm -r azure-iot-pal-arduino
git clone -b make-sdk-python https://github.com/Azure/azure-iot-pal-arduino.git 

cd azure-iot-pal-arduino

git submodule update --init --recursive

rsync -avz --existing ./ sdk/

cd build_all

python3 make_sdk.py -o $ARDUINO_LIBRARY_DIR -d esp32

# input WIFI_SSID, WIFI_PWD, and IOTHUB_CONNECTION_DEVICE_STRING into iot_config.h

cd $ARDUINO_LIBRARY_DIR/AzureIoTHub/examples/iothub_ll_telemetry_sample/
echo $NEWTIN | sudo -S sed -i 's/your-wifi-name/'$WIFI_SSID'/g' *
echo $NEWTIN | sudo -S sed -i 's/your-wifi-pwd/'$WIFI_PWD'/g' *
echo $NEWTIN | sudo -S sed -i 's/your-iothub-DEVICE-connection-strin/'$IOTHUB_CONNECTION_DEVICE_STRING'/g' *
