# Builder
FROM debian:bookworm-slim AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install C++ build tools and libsodium prerequisites (GNU Autotools)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git curl zip unzip tar pkg-config \
    autoconf autoconf-archive automake libtool ca-certificates python3 \
    && rm -rf /var/lib/apt/lists/*

# Clone and bootstrap vcpkg
WORKDIR /opt/vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git . && ./bootstrap-vcpkg.sh

# Install C++ dependencies
RUN ./vcpkg install libsodium mongo-cxx-driver jwt-cpp

WORKDIR /app
COPY CMakeLists.txt .
COPY include/ include/
COPY src/ src/
COPY tests/ tests/

RUN cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release

RUN cmake --build build --target http_server -j$(nproc)

# Runtime
FROM debian:bookworm-slim

# Install basic SSL certs for the MongoDB driver
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/http_server .

EXPOSE 8080

CMD ["./http_server"]