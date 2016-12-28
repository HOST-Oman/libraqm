FROM ubuntu:xenial

RUN apt-get update && apt-get -y install \
    build-essential git  \
    cmake libfreetype6-dev libharfbuzz-dev \
    libfribidi-dev libglib2.0-dev gtk-doc-tools
RUN  ln -s   /usr/bin/gtkdoc-scan /bin/gtkdoc-scan\
     ln -s /usr/bin/gtkdoc-fixxref /bin/gtkdoc-fixxref\
     ln -s /usr/local/bin/gtkdoc-scangobj /bin/gtkdoc-scangobj\
     ln -s /usr/bin/gtkdoc-mkhtml /bin/gtkdoc-mkhtml\
     ln -s /usr/bin/gtkdoc-mkdb /bin/gtkdoc-mkdb

RUN mkdir /build
WORKDIR /build