FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    cmake \
    pkg-config \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-rtsp \
    gstreamer1.0-tools \
    && rm -rf /var/lib/apt/lists/*



    
WORKDIR /app    
COPY . .

RUN mkdir build && cd build && cmake .. && make

CMD ["./build/rtsp_hub"]