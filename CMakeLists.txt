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

# spdlog fetch
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)

# mongodb driver search
find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)

# jwt-cpp search
find_package(jwt-cpp CONFIG REQUIRED)

# Argon2 for cryptography
find_package(unofficial-sodium CONFIG REQUIRED)

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
    src/database/MongoPool.cpp
    src/controllers/UserController.cpp
    src/App.cpp
    src/utils/EnvParser.cpp
    src/utils/Logger.cpp
    src/utils/JwtUtils.cpp
    src/utils/CryptoUtils.cpp
)
target_include_directories(http_server_core PUBLIC include)
target_link_libraries(http_server_core PUBLIC 
    nlohmann_json::nlohmann_json
    spdlog::spdlog_header_only
    jwt-cpp::jwt-cpp
    unofficial-sodium::sodium
    $<IF:$<TARGET_EXISTS:mongo::mongocxx_shared>,mongo::mongocxx_shared,mongo::mongocxx_static>
    $<IF:$<TARGET_EXISTS:mongo::bsoncxx_shared>,mongo::bsoncxx_shared,mongo::bsoncxx_static>)

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