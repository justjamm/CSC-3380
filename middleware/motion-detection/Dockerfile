FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential g++ cmake pkg-config \
    libopencv-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN mkdir build && cd build && cmake .. && make -j$(nproc)

CMD ["./build/src/motion_detection"]
