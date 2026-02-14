cmake_minimum_required(VERSION 3.10)
project(RTSP_Viewer)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV package
find_package(OpenCV REQUIRED)

# Include OpenCV directories
include_directories(${OpenCV_INCLUDE_DIRS})

# Add executable
add_executable(rtsp_viewer rtsp_viewer.cpp)

# Link OpenCV libraries
target_link_libraries(rtsp_viewer ${OpenCV_LIBS})