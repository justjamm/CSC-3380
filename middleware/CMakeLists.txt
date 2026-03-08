cmake_minimum_required(VERSION 3.16)
project(DuncanMiddleware VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Compiler warnings ----
if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ---- Source files ----
set(SOURCES
    src/main.cpp
    src/App.cpp
    src/Server.cpp
)

add_executable(middleware ${SOURCES})

# ---- Include paths ----
target_include_directories(middleware PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ---- Threading (required by httplib + ShutdownManager) ----
find_package(Threads REQUIRED)
target_link_libraries(middleware PRIVATE Threads::Threads)

# ---- Optional: OpenSSL for HTTPS (uncomment when ready) ----
# find_package(OpenSSL REQUIRED)
# target_link_libraries(middleware PRIVATE OpenSSL::SSL OpenSSL::Crypto)
# target_compile_definitions(middleware PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

# ---- Build type defaults ----
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
