
NAME := App_mqtt_azure_client

$(NAME)_SOURCES := azure_mqtt_client.c

$(NAME)_COMPONENTS :=azureiot/azure_c_shared_utility \
					 azureiot/iothub_client \
					 azureiot/mico \
					 azureiot/certs \
					 azureiot/parson
