cmake_minimum_required(VERSION 3.20)

project(cpp_http_server
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(http_server
    src/main.cpp
    src/http/HttpParser.cpp
    src/http/HttpRequest.cpp
    src/network/ClientConnection.cpp
    src/network/ReadBuffer.cpp
    src/network/TcpServer.cpp
    src/network/Socket.cpp
)

target_include_directories(http_server
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)