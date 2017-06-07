#ifndef __MICO_IOT_INIT_H_
#define __MICO_IOT_INIT_H_

#define mico_iot_azure_log(M, ...) custom_log("IoT", M, ##__VA_ARGS__)

bool mico_iot_init(void);

#endif
