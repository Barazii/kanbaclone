cmake_minimum_required(VERSION 3.16)
project(kanba-http-tests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQ REQUIRED libpq)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
find_package(CURL REQUIRED)

set(DOCTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(DBTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../dbtest)

enable_testing()

set(TEST_SOURCES
    test_main.cpp
    test_helpers.cpp
    http_test_client.cpp
    test_health.cpp
    test_auth.cpp
    test_projects.cpp
    test_columns.cpp
    test_tasks.cpp
    test_cors.cpp
    test_ai_chat.cpp
)

add_executable(http_tests ${TEST_SOURCES})

target_include_directories(http_tests PRIVATE
    ${DOCTEST_DIR}
    ${DBTEST_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBPQ_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(http_tests PRIVATE
    ${LIBPQ_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${CURL_LIBRARIES}
)

target_compile_options(http_tests PRIVATE -Wall -Wextra -Wno-unused-parameter)

add_test(NAME http_integration_tests COMMAND http_tests)

add_custom_target(run_http_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS http_tests
)
