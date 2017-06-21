

sdk := ../../../../..

COMPONENT_ADD_INCLUDEDIRS :=  \
$(sdk)/c-utility/inc  \
$(sdk)/c-utility/inc/azure_c_shared_utility \
$(sdk)/c-utility/inc/azure_c_shared_utility/lwip \
$(sdk)/iothub_client/inc \
$(sdk)/umqtt/inc  \
$(sdk)/umqtt/inc/azure_umqtt_c 	\
$(sdk)/parson  \
$(sdk)/iothub_client/samples/iothub_client_sample_mqtt

COMPONENT_OBJS = main.o

COMPONENT_SRCDIRS := .

