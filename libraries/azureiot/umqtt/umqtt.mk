NAME := Lib_iothub_umqtt_Framework


$(NAME)_SOURCES := mqtt_client.c \
				   mqtt_codec.c \
				   mqtt_message.c \
				   mqtt_client.c \		   

$(NAME)_COMPONENTS += azureiot/azure_c_shared_utility
		   
GLOBAL_INCLUDES += . 