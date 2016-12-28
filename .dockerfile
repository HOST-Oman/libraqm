FROM ubuntu:xenial

RUN apt-get update && apt-get -y install \
    build-essential git  \
    cmake libfreetype6-dev libharfbuzz-dev \
    libfribidi-dev libglib2.0-dev gtk-doc-tools

RUN mkdir /build
WORKDIR /build