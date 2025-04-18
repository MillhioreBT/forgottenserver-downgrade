FROM ubuntu:22.04

RUN apt update && \
    apt install -yq cmake build-essential ninja-build \
    libcrypto++-dev libfmt-dev liblua5.4-dev libmysqlclient-dev \
    libboost-iostreams-dev libboost-locale-dev libboost-system-dev libpugixml-dev

COPY cmake /usr/src/forgottenserver/cmake/
COPY src /usr/src/forgottenserver/src/
COPY CMakeLists.txt CMakePresets.json /usr/src/forgottenserver/
WORKDIR /usr/src/forgottenserver

RUN cmake --preset default && cmake --build --config RelWithDebInfo --preset default
