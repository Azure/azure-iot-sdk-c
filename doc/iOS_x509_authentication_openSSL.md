# Enable x509 Authentication on iOS with OpenSSL

Follow these instructions to enable x509 authentication on iOS using OpenSSL. 

Big thanks to user alain-noa who provided these in github isue [#1247](https://github.com/Azure/azure-iot-sdk-c/issues/1247).

## 1. Import OpenSSL library and cURL library to iOS project

You can easily find their source code on github but it's not friendly to generate libraries for iOS. It's easiest to use the [pod library](https://github.com/krzyzanowskim/OpenSSL) for OpenSSL and [framework](https://github.com/Cogosense/iOSCurlFramework/releases) for cURL, just add the pod for OpenSSL and import the framework for cURL in the iOS project.

> It's still recommended to build the libraries from source code if it is comfortable for you.


## 2. Build C SDK for iOS

1. Clone C SDK
- `git clone git@github.com:Azure/azure-iot-sdk-c.git`.

2. Generate an XCode Project, set to use openssl
-    cd C SDK root directory
-   `mkdir cmake`
-   `cd cmake`
-   `cmake -G Xcode -DOPENSSL_ROOT_DIR:PATH=/usr/local/opt/openssl -Duse_openssl:bool=ON ..`

> DOPENSSL_ROOT_DIR is the path of your openssl library in step 1

3. Config build settings
-   Open `azure_iot_sdks.xcodeproj` in cmake folder
-   Edit **Build Settings** for all library targets (`aziotsharedutil`, `iothub_client` ...)
    -  Change **Base SDK** from `macos` to `ios`
    -  Add architectures whatever you need
-   Edit **Search Paths** for target `aziotsharedutil` to link `OpenSSL` library and `cURL` library that  in step 1
    -  If you use static library, add path for header file to **Header Search Paths** and add path for `.a` library to **Library Search Paths**
    -  If you use framework, just add the path to **Framework Search Paths**

> For example, this is you openssl static library, you need to add `../openssl/include` to  **Header Search Paths** and add `../openssl/lib` to **Library Search Paths**
openssl
├── include
│   └── openssl
│       ├── aes.h
│       ├── ... (header files)
└── lib
    ├── libcrypto.a
    └── libssl.a

4. Build C SDK
-   If all the configuration are correct, you should be able to build all the libraries with iOS device or iOS simulator in `azure_iot_sdks.xcodeproj`

## 3. Import C SDK to iOS project
- After step 2, you will get all C SDK libraries you need in /cmake folder in your C SDK project. However,  the headers and static libraries are scattered in different paths, you have to link them to your iOS project.
- You can refer to C SDK sample project to check all the header search path, for example, open `azure_iot_sdks.xcodeproj`, go to **Build Settings** > **Search Paths** of any target for sample (like iothub_II_Client_x509_sample), you can see the search path for each library.
  
You can refer to GitHub issue [#1260](https://github.com/Azure/azure-iot-sdk-c/issues/1260) if there are any build problems.
