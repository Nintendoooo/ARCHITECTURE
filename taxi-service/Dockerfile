FROM ghcr.io/userver-framework/ubuntu-22.04-userver-pg-dev:latest

RUN apt-get update && \
    apt-get install -y \
        libsqlite3-dev \
        sqlite3 \
        wget \
        curl \
        libssl-dev \
        git \
        nlohmann-json3-dev \
        python3 \
        python3-pip \
    && rm -rf /var/lib/apt/lists/*

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
    && cmake -DCMAKE_BUILD_TYPE=Release \
             -DUSERVER_FEATURE_POSTGRESQL=OFF \
             -DUSERVER_FEATURE_TESTSUITE=OFF \
             -DUSERVER_FEATURE_FORMAT=OFF \
             .. \
    && make -j2

RUN mkdir -p /app/data \
    && cp /app/build/taxi-service-input   /app/taxi-service-input \
    && cp /app/build/taxi-service-order   /app/taxi-service-order \
    && cp /app/build/taxi-service-match   /app/taxi-service-match \
    && chmod +x /app/taxi-service-input \
                /app/taxi-service-order \
                /app/taxi-service-match

HEALTHCHECK --interval=30s --timeout=10s --start-period=10s --retries=3 \
    CMD wget --quiet --tries=1 --spider http://localhost:8081/ping || exit 1

CMD ["/bin/bash"]
