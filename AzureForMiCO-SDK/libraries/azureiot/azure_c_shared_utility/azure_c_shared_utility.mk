NAME := Lib_iothub_shared_util_Framework


$(NAME)_SOURCES := agenttime.c \
				   base64.c \
				   buffer.c \
				   connection_string_parser.c \
				   consolelogger.c \
				   constbuffer.c \
				   constmap.c \
				   crt_abstractions.c \
				   doublylinkedlist.c \
				   gb_stdio.c \
				   gb_time.c \
				   gballoc.c \
				   hmac.c \
				   hmacsha256.c \
				   httpapiex.c \
				   httpapiexsas.c \
				   httpheaders.c \
				   map.c \
				   optionhandler.c \
				   platform_mico.c \
				   sastoken.c \
				   sha1.c \
				   sha224.c \
				   sha384-512.c \
				   singlylinkedlist.c \
				   string_tokenizer.c \
				   strings.c \
				   tickcounter.c \
				   tlsio_mico.c \
				   urlencode.c \
				   usha.c \
				   vector.c \
				   xio.c \
				   xlogging.c \
				   httpapi_mico.c			   

$(NAME)_COMPONENTS :=azureiot/parson
	   
GLOBAL_INCLUDES += . 