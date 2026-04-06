cmake_minimum_required(VERSION 3.16)
project(DuncanMiddleware VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Compiler warnings ----
if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ---- Source files ----
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_executable(middleware ${SOURCES})

# ---- Include paths ----
target_include_directories(middleware PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ---- Threading (required by httplib + ShutdownManager) ----
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(jwt-cpp QUIET)
if(NOT jwt-cpp_FOUND)
    include(FetchContent)
    set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(JWT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        jwt-cpp
        GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
        GIT_TAG v0.7.1
    )
    FetchContent_MakeAvailable(jwt-cpp)
endif()

target_link_libraries(middleware PRIVATE
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    jwt-cpp::jwt-cpp
)

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
