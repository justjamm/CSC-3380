cmake_minimum_required(VERSION 3.10)
project(rtsp_hub)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)

include_directories(
    ${GSTREAMER_INCLUDE_DIRS}
    third_party/cpp-httplib
)

add_executable(rtsp_hub src/main.cpp)

target_link_libraries(rtsp_hub
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_APP_LIBRARIES}
    Threads::Threads
)

target_compile_options(rtsp_hub PRIVATE -Wall -Wextra)