cmake_minimum_required(VERSION 3.20)

project(HighPerformanceHttpServer
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# JSON fetch
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(json)

# Main
add_library(http_server_core
    src/http/HttpHeaders.cpp
    src/http/HttpParser.cpp
    src/http/HttpRequest.cpp
    src/http/HttpResponse.cpp
    src/http/HttpUtils.cpp
    src/http/Router.cpp
    src/network/ClientConnection.cpp
    src/network/ReadBuffer.cpp
    src/network/TcpServer.cpp
    src/network/Socket.cpp
    src/network/Epoll.cpp
    src/concurrency/ThreadPool.cpp
)
target_include_directories(http_server_core PUBLIC include)
target_link_libraries(http_server_core PUBLIC nlohmann_json::nlohmann_json)

# main exec
add_executable(http_server src/main.cpp)
target_link_libraries(http_server PRIVATE http_server_core)

# GTEST
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_subdirectory(tests)