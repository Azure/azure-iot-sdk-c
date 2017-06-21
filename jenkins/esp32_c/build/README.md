# ESP32 Gate Build

This project exercises the ESP32 cross-compiler against the Azure IoT C SDK. This project is Linux-only, and is built by the `linux_esp32_c.sh` script in this project's parent directory.

### Project environment
The ESP32 build tools expect two pieces of environment information
* **IDF_PATH** - this export must be set to the directory containing the ESP32 SDK
* **ESP32_TOOLS** - this export must contain the ESP32 toolchain, which the `linux_esp32_c.sh` script will add to the PATH

### Project structure
The ESP32 build tools expect the project to be contained in two directories:
* **components** - each subdirectory containing a component.mk file will be built and treated as a library
* **main** - the contents of main are treated as project files, and must contain an app_main() function

### Project setup

Here are the instructions for setting up the Linux environment for the ESP32 cross-compiler:

##### Install prerequisites:  

`sudo apt-get install git wget make libncurses-dev flex bison gperf python python-serial`


##### Install the ESP32 toolchain:

Extract:
https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-61-gab8375a-5.2.0.tar.gz
Into /home/jenkins/esp32: 

`mkdir -p /home/jenkins/esp32`

`cd /home/jenkins/esp32`

`tar -xzf ~/Downloads/xtensa-esp32-elf-linux64-1.22.0-61-gab8375a-5.2.0.tar.gz`

##### Create an export for the toolchain location:<br/>
`export ESP32_TOOLS="/home/jenkins/esp32"`

##### Install the ESP32 SDK:

`cd /home/jenkins` 

`git clone --recursive https://github.com/espressif/esp-idf.git esp32-idf`

`git checkout 53893297299e207029679dc99b7fb33151bdd415`

##### Export the ESP32 SDK location:

`export IDF_PATH="/home/jenkins/esp32-idf"`
