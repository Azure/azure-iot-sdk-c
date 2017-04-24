This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

# network_e2e tests for the Azure IOT SDK library for C



# Test purpose

These tests are used to vaildate the behavior of the Azure IoT SDK library in network condition with intermitent connectivity. 

## Test environment

These tests run inside Docker containers.  There is a single container (the test controller) which is responsible for controlling the tests, and multiple containers (containers under test) responsible for running the tests (one container per test).  Docker-compose is used to manage the containers under test.  The containers under test can use the docker executable to control the test controller via unix socket (under Linux) or IP socket (under Windows).

The test controller doesn't necessarily have to be a container.  The tests controller scripts can run within the host OS if convenient (e.g. for developer debugging)

## Preparing the Linux environment (one-time)

The following needs to be installed on the test controller, whether it be the host OS or a container:

Docker needs to be installed:
```
curl -sSL https://get.docker.com/ | sh
sudo usermod -aG docker $USER
```

Docker-compose needs to be installed:
```
sudo apt-get -y install python-pip 
sudo -H pip install --upgrade pip 
sudo -H pip install docker-compose 
```
 
## Preparing the Linux environment (per run)
To build the binaries and images, run prepare.sh

If you get permission denied errors when running prepare.sh, you may need to manually remove the cmake folder:
```
user@machine:/repos/c/network_e2e$ ./prepare.sh
rm: cannot remove '/repos/c/cmake/linux_network_e2e/network_e2e/tests/iothubclient_badnetwork_e2e/Testing/Temporary/LastTest.log': Permission denied
user@machine:/repos/c/network_e2e$sudo rm -r /repos/c/cmake/linux_network_e2e/
```

## Running all tests in Docker-compose on Linux
To run all tests using docker.compose, execute the run_all_in_compose.sh script

## Running an individual test inside a docker container on Linux

To run all tests using docker.compose, execute the run_individual_in_container.sh script passing the protocol as a parameter:
```
user@machine:/repos/c/network_e2e$ ./run_individual_in_container.sh AMQP
running /bin/bash /repos/c/network_e2e/rt_container.sh AMQP
```

