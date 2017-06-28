#
# Component Makefile
#

# Component configuration in preprocessor defines
CFLAGS += -DUSE_LWIP_SOCKET_FOR_AZURE_IOT 

sdk := ../../../..

COMPONENT_ADD_INCLUDEDIRS :=  \
$(sdk)/c-utility/inc  \
$(sdk)/c-utility/inc/azure_c_shared_utility \
$(sdk)/c-utility/inc/azure_c_shared_utility/lwip \
$(sdk)/iothub_client/inc \
$(sdk)/umqtt/inc  \
$(sdk)/umqtt/inc/azure_umqtt_c 	\
$(sdk)/parson  \
$(sdk)/iothub_client/samples/iothub_client_sample_mqtt

COMPONENT_OBJS =  \
$(sdk)/c-utility/src/xlogging.o	\
$(sdk)/c-utility/src/buffer.o	\
$(sdk)/c-utility/src/consolelogger.o	\
$(sdk)/c-utility/src/constbuffer.o	\
$(sdk)/c-utility/src/constmap.o	\
$(sdk)/c-utility/src/crt_abstractions.o	\
$(sdk)/c-utility/src/doublylinkedlist.o	\
$(sdk)/c-utility/src/gballoc.o	\
$(sdk)/c-utility/src/gb_stdio.o	\
$(sdk)/c-utility/src/gb_time.o	\
$(sdk)/c-utility/src/hmac.o	\
$(sdk)/c-utility/src/hmacsha256.o	\
$(sdk)/c-utility/src/httpapiex.o	\
$(sdk)/c-utility/src/httpapiexsas.o	\
$(sdk)/c-utility/src/httpheaders.o	\
$(sdk)/c-utility/src/map.o	\
$(sdk)/c-utility/src/optionhandler.o	\
$(sdk)/c-utility/src/sastoken.o	\
$(sdk)/c-utility/src/sha1.o	\
$(sdk)/c-utility/src/sha224.o	\
$(sdk)/c-utility/src/sha384-512.o	\
$(sdk)/c-utility/src/strings.o	\
$(sdk)/c-utility/src/string_tokenizer.o	\
$(sdk)/c-utility/src/urlencode.o	\
$(sdk)/c-utility/src/usha.o	\
$(sdk)/c-utility/src/vector.o	\
$(sdk)/c-utility/src/xio.o	\
$(sdk)/c-utility/src/base64.o \
\
\
$(sdk)/iothub_client/src/iothub_client.o	\
$(sdk)/iothub_client/src/iothub_client_ll.o	\
$(sdk)/iothub_client/src/iothub_client_ll_uploadtoblob.o	\
$(sdk)/iothub_client/src/iothub_client_authorization.o	\
$(sdk)/iothub_client/src/iothub_message.o	\
$(sdk)/iothub_client/src/iothubtransport.o	\
$(sdk)/iothub_client/src/iothubtransportmqtt.o	\
$(sdk)/iothub_client/src/iothubtransport_mqtt_common.o	\
$(sdk)/iothub_client/src/version.o	\
\
\
$(sdk)/umqtt/src/mqtt_client.o	\
$(sdk)/umqtt/src/mqtt_codec.o	\
$(sdk)/umqtt/src/mqtt_message.o	\
\
\
\
$(sdk)/c-utility/adapters/agenttime.o	\
$(sdk)/c-utility/src/singlylinkedlist.o	\
$(sdk)/c-utility/adapters/platform_openssl_compact.o	\
$(sdk)/c-utility/adapters/threadapi_freertos.o	\
$(sdk)/c-utility/adapters/tickcounter_freertos.o	\
$(sdk)/c-utility/adapters/sntp_lwip.o	\
$(sdk)/c-utility/adapters/dns_async.o	\
$(sdk)/c-utility/adapters/socket_async.o	\
$(sdk)/c-utility/adapters/tlsio_openssl_compact.o	\
$(sdk)/iothub_client/samples/iothub_client_sample_mqtt/iothub_client_sample_mqtt.o	\


COMPONENT_SRCDIRS :=  \
$(sdk)/c-utility/src \
$(sdk)/c-utility/adapters  \
$(sdk)/umqtt/src	\
$(sdk)/iothub_client/src  \
$(sdk)/iothub_client/samples/iothub_client_sample_mqtt  \
$(sdk)/parson	\
















