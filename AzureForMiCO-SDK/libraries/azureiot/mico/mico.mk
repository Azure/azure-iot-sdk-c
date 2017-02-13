NAME := Lib_iothub_mico


$(NAME)_SOURCES := mico_time.c		   

$(NAME)_COMPONENTS += protocols/SNTP
		   
GLOBAL_INCLUDES += . 