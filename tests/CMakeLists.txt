add_executable(http_server_tests
    HttpRequestTest.cpp
)

target_link_libraries(http_server_tests
    PRIVATE
    http_server_core
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(http_server_tests)