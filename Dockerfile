FROM gcc:10.2.0

ENV CPPZMQ_VERSION=4.6.0
ENV PROTOBUF_VERSION=3.12.3

ARG ARROW_DEB_PCKG_SHA256
ENV ENV_ARROW_DEB_PCKG_SHA256=$ARROW_DEB_PCKG_SHA256

ARG CPPZMQ_SHA256
ENV ENV_CPPZMQ_SHA256=$CPPZMQ_SHA256

ARG PROTOBUF_SHA256
ENV ENV_PROTOBUF_SHA256=$PROTOBUF_SHA256

# Configure system for further build and run
RUN apt-get update \
    && apt-get install -y -V autoconf automake libtool curl g++ unzip cmake make wget tar pkg-config ca-certificates lsb-release git libzmq3-dev \
    && if [ "${ENV_ARROW_DEB_PCKG_SHA256}" = "" ]; then echo "arrow deb package sha256 hash sum environment variable is empty. Exiting..." ; exit 1 ; fi \
    && wget https://apache.bintray.com/arrow/debian/apache-arrow-archive-keyring-latest-buster.deb \
    && if [ "$(sha256sum apache-arrow-archive-keyring-latest-buster.deb)" != "${ENV_ARROW_DEB_PCKG_SHA256}" ]; then echo "Bad SHA256 hash sum of apache-arrow-archive-keyring-latest-buster.deb" ; exit 1 ; fi \
    && apt-get install -y -V ./apache-arrow-archive-keyring-latest-$(lsb_release --codename --short).deb \
    && apt-get update \
    && apt-get install -y -V libarrow-dev libgandiva-dev \
    && git clone https://github.com/gabime/spdlog.git \
    && cd spdlog && mkdir build && cd build \
    && cmake .. && make -j install && cd ../.. \
    && if [ "${ENV_CPPZMQ_SHA256}" = "" ]; then echo "cppzmq sha256 hash sum environment variable is empty. Exiting..." ; exit 1 ; fi \
    && wget https://github.com/zeromq/cppzmq/archive/v${CPPZMQ_VERSION}.tar.gz \
    && if [ "$(sha256sum v${CPPZMQ_VERSION}.tar.gz)" != "${ENV_CPPZMQ_SHA256}" ]; then echo "Bad SHA256 hash sum of v${CPPZMQ_VERSION}.tar.gz" ; exit 1 ; fi \
    && tar -xvzf v${CPPZMQ_VERSION}.tar.gz && cd cppzmq-${CPPZMQ_VERSION} && mkdir build && cd build && cmake .. && make -j4 install && cd ../.. \
    && git clone https://github.com/catchorg/Catch2.git \
    && cd Catch2 \
    && cmake -Bbuild -H. -DBUILD_TESTING=OFF \
    && cmake --build build/ --target install && cd .. \
    && if [ "${ENV_PROTOBUF_SHA256}" = "" ]; then echo "protobuf sha256 hash sum environment variable is empty. Exiting..." ; exit 1 ; fi \
    && wget https://github.com/protocolbuffers/protobuf/releases/download/v${PROTOBUF_VERSION}/protobuf-cpp-${PROTOBUF_VERSION}.tar.gz \
    && if [ "$(sha256sum protobuf-cpp-3.12.3.tar.gz)" != "${ENV_PROTOBUF_SHA256}" ]; then echo "Bad SHA256 hash sum of protobuf-cpp-3.12.3.tar.gz" ; exit 1 ; fi \
    && tar -xzvf --no-same-owner protobuf-cpp-${PROTOBUF_VERSION}.tar.gz \
    && cd protobuf-${PROTOBUF_VERSION} && ./configure && make && make check && make install && ldconfig
