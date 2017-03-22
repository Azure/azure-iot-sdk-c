############################################################
# Dockerfile for Linux network E2E tests
# Based on Ubuntu-16.04
############################################################

# Set the base image to Ubuntu
FROM aziotbld/linux-c-ubuntu-16.04

# Add tools for network connect/disconnect tests
RUN apt-get install --fix-missing -y \
inetutils-ping \
inetutils-traceroute \
net-tools \
iptables \
sudo \
curl

# add jenkins user to sudoers so he can tweak network connections
RUN usermod -aG sudo jenkins

# jenkins user doesn't need password to run these commands (makes scripting much easier)
# must have tab character between first and second word, else syntax error
RUN echo "jenkins	ALL=(ALL:ALL) NOPASSWD:/sbin/ip,/sbin/iptables,/sbin/route" > /etc/sudoers.d/jenkins

# install docker client.  make sure user has access to /var/run/docker.sock
# if you see the following error, you need to add -v /var/run/docker.sock:/var/run/docker/sock to your run command
RUN apt-get -yqq update && apt-get -yqq install docker.io && usermod -aG docker jenkins
VOLUME /var/run/docker.sock

VOLUME /src

