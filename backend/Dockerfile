# Build stage
FROM ubuntu:22.04 AS builder

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    git \
    pkg-config \
    libpq-dev \
    libssl-dev \
    zlib1g-dev \
    libjsoncpp-dev \
    uuid-dev \
    libsodium-dev \
    libc-ares-dev \
    libhiredis-dev \
    libmariadb-dev \
    libsqlite3-dev \
    libbrotli-dev \
    && rm -rf /var/lib/apt/lists/*

# Clone and build Drogon
WORKDIR /opt
RUN git clone --depth 1 --branch v1.9.3 https://github.com/drogonframework/drogon.git \
    && cd drogon \
    && git submodule update --init \
    && mkdir build \
    && cd build \
    && cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_POSTGRESQL=ON \
        -DBUILD_MYSQL=OFF \
        -DBUILD_SQLITE=OFF \
        -DBUILD_REDIS=OFF \
    && make -j$(nproc) \
    && make install

# Copy source code
WORKDIR /app
COPY . .

# Build the application
RUN mkdir -p build \
    && cd build \
    && cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
    && make -j$(nproc)

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libpq5 \
    libssl3 \
    zlib1g \
    libjsoncpp25 \
    libuuid1 \
    libsodium23 \
    libc-ares2 \
    libbrotli1 \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Copy the built binary
COPY --from=builder /app/build/kanba-backend /app/kanba-backend

# Copy config files
COPY --from=builder /app/config.json /app/config.json

# Create logs directory
RUN mkdir -p /app/logs

WORKDIR /app

# Set environment variables
ENV DATABASE_HOST=localhost \
    DATABASE_PORT=5432 \
    DATABASE_NAME=kanba \
    DATABASE_USER=postgres \
    DATABASE_PASSWORD="" \
    PORT=3001 \
    NODE_ENV=production \
    FRONTEND_URL=""

EXPOSE 3001

CMD ["./kanba-backend"]
