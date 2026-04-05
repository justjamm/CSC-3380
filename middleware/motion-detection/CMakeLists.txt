cmake_minimum_required(VERSION 3.10)
project(motion_detection)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    third_party/cpp-httplib
    third_party/nlohmann
)

add_subdirectory(src)
