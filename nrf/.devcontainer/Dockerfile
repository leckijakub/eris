FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
ENV SDK_URL="https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/sdks/nrf5/binaries/nrf5sdk160098a08e2.zip"
ENV SDK_DIR=/root/nrf5_sdk/

RUN apt update && \
    apt -y upgrade && \
    apt install -y gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch openocd wget unzip make python3 openssh-client git sshpass

# Prepare NRF5 SDK
RUN wget ${SDK_URL} && \
    unzip $(basename ${SDK_URL}) -d ${SDK_DIR}

# update the path to GNU binaries
RUN sed -i "s@GNU_INSTALL_ROOT ?= .*@GNU_INSTALL_ROOT ?= /usr/bin/@g" ${SDK_DIR}/components/toolchain/gcc/Makefile.posix

WORKDIR ${SDK_DIR}/examples/
