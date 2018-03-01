FROM ubuntu

# Add an unprivileged user account for running the module
RUN useradd -ms /bin/bash moduleuser
ENV ModuleUser=moduleuser

RUN apt-get update && apt-get install -y libcurl3

WORKDIR /app
ADD iothub_client_sample_module_sender /app
ADD start.sh /app

CMD ["/app/start.sh"]
