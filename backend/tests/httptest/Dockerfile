FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake g++ pkg-config \
    libcurl4-openssl-dev \
    libjsoncpp-dev \
    libpq-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tests

COPY backend/tests/doctest.h /tests/doctest.h
COPY backend/tests/httptest/ /tests/httptest/
COPY backend/tests/dbtest/db_test_helper.h /tests/dbtest/db_test_helper.h

WORKDIR /tests/httptest/build
RUN cmake .. && make -j$(nproc)

CMD ["ctest", "--output-on-failure"]
