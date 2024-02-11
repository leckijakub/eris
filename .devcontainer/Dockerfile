FROM ubuntu:20.04

ARG USERNAME=eris
ARG USER_UID=1000
ARG USER_GID=$USER_UID
ARG SDK_URL="https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/sdks/nrf5/binaries/nrf5sdk160098a08e2.zip"
ARG SDK_DIR=/home/$USERNAME/nrf5_sdk/

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get -y upgrade && \
    apt-get install -y gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch openocd wget unzip make python3 python3-pip openssh-client git sshpass curl sudo vim clang-format-11

RUN pip3 install nrfutil

# change root password

# # Create a non-root user
RUN groupadd -g $USER_GID $USERNAME && \
    useradd -m -u $USER_UID -g $USER_GID -s /bin/bash $USERNAME && \
    usermod -aG sudo $USERNAME
RUN echo 'eris:eris' | chpasswd
USER $USERNAME

WORKDIR /home/$USERNAME
# Prepare NRF5 SDK
RUN wget ${SDK_URL} && \
    unzip $(basename ${SDK_URL}) -d ${SDK_DIR}

# update the path to GNU binaries
RUN sed -i "s@GNU_INSTALL_ROOT ?= .*@GNU_INSTALL_ROOT ?= /usr/bin/@g" ${SDK_DIR}/components/toolchain/gcc/Makefile.posix

WORKDIR ${SDK_DIR}/examples/
