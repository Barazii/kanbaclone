cmake_minimum_required(VERSION 3.16)
project(kanba-db-contract-tests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)

set(DOCTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

enable_testing()

function(add_db_test TEST_NAME TEST_SRC)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_include_directories(${TEST_NAME} PRIVATE
        ${DOCTEST_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${LIBPQXX_INCLUDE_DIRS}
    )
    target_link_libraries(${TEST_NAME} PRIVATE ${LIBPQXX_LIBRARIES})
    target_compile_options(${TEST_NAME} PRIVATE -Wall -Wextra -Wno-unused-parameter)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_db_test(test_db_user_functions    test_user_functions.cpp)
add_db_test(test_db_project_functions test_project_functions.cpp)
add_db_test(test_db_column_functions  test_column_functions.cpp)
add_db_test(test_db_task_functions    test_task_functions.cpp)
add_db_test(test_db_member_functions  test_member_functions.cpp)

add_custom_target(run_db_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS
        test_db_user_functions
        test_db_project_functions
        test_db_column_functions
        test_db_task_functions
        test_db_member_functions
    COMMENT "Running database contract tests"
)
