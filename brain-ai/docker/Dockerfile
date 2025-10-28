# Multi-stage build for Brain-AI v3.6.0

# Stage 1: Builder
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y \
    build-essential cmake ninja-build pkg-config g++ git \
    libeigen3-dev libprotobuf-dev protobuf-compiler \
    protobuf-compiler-grpc libgrpc++-dev \
    libssl-dev libsqlite3-dev libcurl4-openssl-dev \
    libyaml-cpp-dev libopenblas-dev liblapack-dev \
    && rm -rf /var/lib/apt/lists/*

# Install FAISS
WORKDIR /tmp
RUN git clone --depth 1 https://github.com/facebookresearch/faiss.git && \
    cd faiss && \
    cmake -B build -DCMAKE_BUILD_TYPE=Release \
          -DFAISS_ENABLE_GPU=OFF -DFAISS_ENABLE_PYTHON=OFF \
          -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=ON \
          -DBLA_VENDOR=OpenBLAS && \
    cmake --build build -j$(nproc) && \
    cmake --install build

# Install HNSWlib
RUN git clone --depth 1 https://github.com/nmslib/hnswlib.git && \
    mkdir -p /usr/local/include/hnswlib && \
    cp hnswlib/hnswlib/*.h /usr/local/include/hnswlib/

# Build application
WORKDIR /app
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF && \
    cmake --build build -j$(nproc)

# Stage 2: Runtime
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    libprotobuf32 libgrpc++1.51 libssl3 libsqlite3-0 \
    libyaml-cpp0.7 libopenblas0 libgomp1 \
    && rm -rf /var/lib/apt/lists/*

# Copy FAISS library
COPY --from=builder /usr/local/lib/libfaiss.so /usr/local/lib/
RUN ldconfig

# Copy application
COPY --from=builder /app/build/brain_server /usr/local/bin/
COPY --from=builder /app/build/brain_search /usr/local/bin/
COPY --from=builder /app/build/brain_reindex /usr/local/bin/
COPY --from=builder /app/configs /app/configs

# Create non-root user
RUN useradd -m -u 1000 brain && \
    mkdir -p /app/data && \
    chown -R brain:brain /app

USER brain
WORKDIR /app

EXPOSE 50051 8080

CMD ["brain_server", "configs/system.yaml"]
