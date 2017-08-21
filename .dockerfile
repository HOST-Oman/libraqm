FROM ubuntu:xenial

RUN apt-get update && apt-get -y install \
    build-essential git  \
    cmake libfreetype6-dev libharfbuzz-dev \
    libfribidi-dev libglib2.0-dev gtk-doc-tools
RUN ln -s /usr/bin/gtkdoc-scan /bin/
RUN ln -s /usr/bin/gtkdoc-fixxref /bin/
RUN ln -s /usr/bin/gtkdoc-scangobj /bin/
RUN ln -s /usr/bin/gtkdoc-mkhtml /bin/
RUN ln -s /usr/bin/gtkdoc-mkdb /bin/

RUN mkdir /build
WORKDIR /build