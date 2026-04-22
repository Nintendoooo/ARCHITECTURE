FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install base build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    pkg-config \
    ccache \
    ninja-build \
    python3 \
    python3-pip \
    python3-venv \
    python3-jinja2 \
    python3-yaml \
    python3-voluptuous \
    # userver core dependencies
    libboost-all-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    libev-dev \
    libc-ares-dev \
    libnghttp2-dev \
    libfmt-dev \
    libspdlog-dev \
    libyaml-cpp-dev \
    libhttp-parser-dev \
    zlib1g-dev \
    libbenchmark-dev \
    libgtest-dev \
    libgmock-dev \
    libcrypto++-dev \
    liblz4-dev \
    libzstd-dev \
    libjemalloc-dev \
    # JSON
    nlohmann-json3-dev \
    # mongo-c-driver build deps
    libsasl2-dev \
    libsnappy-dev \
    # Install apt version first to get pkg-config files, then build from source
    libmongoc-dev \
    libbson-dev \
    && rm -rf /var/lib/apt/lists/*

# Build and install mongo-c-driver (libbson + libmongoc) from source
# userver requires >= 1.16.0; we use 1.24.0 (compatible with mongo-cxx-driver r3.7.0)
RUN cd /tmp && \
    wget -q https://github.com/mongodb/mongo-c-driver/releases/download/1.24.0/mongo-c-driver-1.24.0.tar.gz && \
    tar xzf mongo-c-driver-1.24.0.tar.gz && \
    cmake -S /tmp/mongo-c-driver-1.24.0 -B /tmp/mongo-c-driver-build \
          -DCMAKE_BUILD_TYPE=Release \
          -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF \
          -DENABLE_TESTS=OFF \
          -DENABLE_EXAMPLES=OFF \
          -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build /tmp/mongo-c-driver-build -j2 && \
    cmake --install /tmp/mongo-c-driver-build && \
    ldconfig && \
    rm -rf /tmp/mongo-c-driver-1.24.0* /tmp/mongo-c-driver-build

# Build and install mongo-cxx-driver (C++ driver)
# Use r3.7.0 which is compatible with mongo-c-driver 1.24.0
# Use limited parallelism (j2) and disable tests to avoid OOM
RUN cd /tmp && \
    wget -q https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.7.0/mongo-cxx-driver-r3.7.0.tar.gz && \
    tar xzf mongo-cxx-driver-r3.7.0.tar.gz && \
    cd mongo-cxx-driver-r3.7.0 && \
    mkdir -p build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DBSONCXX_POLY_USE_MNMLSTC=1 \
          -DENABLE_TESTS=OFF \
          .. && \
    cmake --build . -j2 && \
    cmake --install . && \
    ldconfig && \
    cd /tmp && \
    rm -rf mongo-cxx-driver-r3.7.0*

RUN cd /tmp \
    && git clone --depth 1 --branch v0.7.0 https://github.com/Thalhammer/jwt-cpp.git \
    && cd jwt-cpp \
    && mkdir -p build && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release \
             -DJWT_BUILD_EXAMPLES=OFF \
             -DJWT_BUILD_TESTS=OFF \
             -DJWT_INSTALL=ON \
             -DCMAKE_INSTALL_PREFIX=/usr/local \
             -DDEFAULT_NLOHMANN_JSON_BACKEND=OFF \
             .. \
    && make install \
    && ldconfig \
    && rm -rf /tmp/jwt-cpp

WORKDIR /app

COPY . /app

RUN mkdir -p build && cd build \
    && PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/local/lib/aarch64-linux-gnu/pkgconfig:/usr/local/lib64/pkgconfig:/usr/local/lib/x86_64-linux-gnu/pkgconfig \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_PREFIX_PATH=/usr/local \
          .. \
    && cmake --build . -j$(nproc)

RUN cp /app/build/taxi-service-input   /app/taxi-service-input \
    && cp /app/build/taxi-service-order   /app/taxi-service-order \
    && cp /app/build/taxi-service-match   /app/taxi-service-match \
    && chmod +x /app/taxi-service-input \
                /app/taxi-service-order \
                /app/taxi-service-match

HEALTHCHECK --interval=30s --timeout=10s --start-period=10s --retries=3 \
    CMD wget --quiet --tries=1 --spider http://localhost:8081/ping || exit 1
