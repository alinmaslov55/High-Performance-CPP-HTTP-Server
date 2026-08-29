cmake_minimum_required(VERSION 3.20)

project(cpp_http_server
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP true
)

FetchContent_MakeAvailable(json)

add_executable(http_server
    src/main.cpp
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
)
target_link_libraries(http_server
    PRIVATE
    nlohmann_json::nlohmann_json
)

target_include_directories(http_server
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)