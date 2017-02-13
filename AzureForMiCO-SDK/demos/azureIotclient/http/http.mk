
NAME := App_http_azure_client

$(NAME)_SOURCES := azure_http_client.c \
                   mico_iot_init.c

$(NAME)_COMPONENTS :=azureiot/azure_c_shared_utility \
					 azureiot/iothub_client