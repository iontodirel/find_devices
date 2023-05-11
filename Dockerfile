FROM ubuntu:22.04

RUN apt-get update -y

RUN apt-get install -y git

RUN git clone https://github.com/iontodirel/find_devices.git

WORKDIR /find_devices

RUN ./install_dependencies.sh

RUN cmake .
RUN make -j 4
