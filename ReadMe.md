# RTSP Stream Viewer

A simple C++ program that captures an RTSP stream and displays it using OpenCV.

## Prerequisites

- C++ compiler (g++ or clang++)
- OpenCV 3.x or 4.x with FFmpeg support
- CMake (optional, for CMake build method)

### Installing OpenCV on Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install libopencv-dev
```

### Installing OpenCV on macOS:
```bash
brew install opencv
```

## Compilation

### Method 1: Using Makefile (recommended for simple builds)
```bash
make
```

### Method 2: Using CMake
```bash
mkdir build
cd build
cmake ..
make
```

### Method 3: Direct compilation
```bash
g++ -std=c++11 rtsp_viewer.cpp -o rtsp_viewer `pkg-config --cflags --libs opencv4`
```

## Usage

### Run with default RTSP URL (edit in source code):
```bash
./rtsp_viewer
```

### Run with custom RTSP URL:
```bash
./rtsp_viewer rtsp://username:password@192.168.1.100:554/stream
```

### Example RTSP URLs:
```bash
# Generic IP camera
./rtsp_viewer rtsp://192.168.1.100:554/stream1

# With authentication
./rtsp_viewer rtsp://admin:password@192.168.1.100:554/stream1

# Test streams (if available)
./rtsp_viewer rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mp4
```

## Controls

- Press `q` or `ESC` to quit the application

## Troubleshooting

### Cannot open stream
- Verify the RTSP URL is correct
- Check network connectivity to the camera/server
- Ensure the camera/server is powered on and accessible
- Try the URL in VLC player first to verify it works

### Compilation errors
- Make sure OpenCV is properly installed
- Check that pkg-config can find OpenCV: `pkg-config --modversion opencv4`
- If using OpenCV 3.x, change `opencv4` to `opencv` in compilation commands

### Empty frames or stream drops
- Network latency may be too high
- The stream may have encoding issues
- Try reducing the resolution or bitrate on the camera side

## Features

- Displays RTSP stream in real-time
- Shows stream properties (resolution, FPS)
- Handles connection errors gracefully
- Supports command-line RTSP URL input
- Clean resource management

## Notes

- The program uses FFmpeg backend for RTSP streaming
- Press Ctrl+C in terminal to force quit if window controls don't work
- Some cameras may require specific codec support in OpenCV build