cmake_minimum_required(VERSION 3.16)
project(kanba-backend-tests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Doctest is header-only, included in the tests/ directory
# No external test framework installation required.

# ---- Mock include paths ----
# The mock headers override drogon, json, uuid, and sodium so that
# unit tests can compile and run without installing those libraries.
set(MOCK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mocks)

# Source directory for the production code
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# ---- Common compile settings ----
# Important: mock include dir comes BEFORE system include dirs so that
# #include <drogon/drogon.h> resolves to our mock, not the real library.
function(add_kanba_test TEST_NAME TEST_SRC)
    add_executable(${TEST_NAME} ${TEST_SRC} ${ARGN})
    target_include_directories(${TEST_NAME} PRIVATE
        ${MOCK_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${SRC_DIR}
    )
    # -Wno-shadow: production code has a known shadow in AiChatController.cpp
    # -Wno-unused-parameter: Drogon callback signatures often have unused params
    target_compile_options(${TEST_NAME} PRIVATE -Wall -Wextra -Wno-shadow -Wno-unused-parameter)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

enable_testing()

# ===========================================================================
# Unit Tests
# ===========================================================================

# 1. PasswordHash tests
add_kanba_test(test_password_hash
    unit/test_password_hash.cpp
    ${SRC_DIR}/utils/PasswordHash.cpp
)

# 2. Session tests (uuid generation + logic)
add_kanba_test(test_session
    unit/test_session.cpp
    ${SRC_DIR}/utils/Session.cpp
    ${SRC_DIR}/utils/Database.cpp
)

# 3. Database utility tests
add_kanba_test(test_database
    unit/test_database.cpp
    ${SRC_DIR}/utils/Database.cpp
)

# 4. CorsFilter tests
add_kanba_test(test_cors_filter
    unit/test_cors_filter.cpp
    ${SRC_DIR}/filters/CorsFilter.cpp
)

# 5. AuthFilter tests
add_kanba_test(test_auth_filter
    unit/test_auth_filter.cpp
    ${SRC_DIR}/filters/AuthFilter.cpp
    ${SRC_DIR}/utils/Session.cpp
    ${SRC_DIR}/utils/Database.cpp
)

# 6. HealthController tests
add_kanba_test(test_health_controller
    unit/test_health_controller.cpp
    ${SRC_DIR}/controllers/HealthController.cpp
)

# 7. AuthController tests
add_kanba_test(test_auth_controller
    unit/test_auth_controller.cpp
    ${SRC_DIR}/controllers/AuthController.cpp
    ${SRC_DIR}/utils/PasswordHash.cpp
    ${SRC_DIR}/utils/Session.cpp
    ${SRC_DIR}/utils/Database.cpp
)

# 8. ProjectController tests
add_kanba_test(test_project_controller
    unit/test_project_controller.cpp
    ${SRC_DIR}/controllers/ProjectController.cpp
    ${SRC_DIR}/utils/Database.cpp
)

# 9. ColumnController tests
add_kanba_test(test_column_controller
    unit/test_column_controller.cpp
    ${SRC_DIR}/controllers/ColumnController.cpp
    ${SRC_DIR}/utils/Database.cpp
)

# 10. TaskController tests
add_kanba_test(test_task_controller
    unit/test_task_controller.cpp
    ${SRC_DIR}/controllers/TaskController.cpp
    ${SRC_DIR}/utils/Database.cpp
)

# 11. AiChatController tests
# Note: AiChatController.cpp has a GCC 13+ shadow error (lambda param
# 'result' shadowed by local 'result'). We use a patched copy for tests.
add_kanba_test(test_ai_chat_controller
    unit/test_ai_chat_controller.cpp
    unit/AiChatController_patched.cpp
)

# ===========================================================================
# Integration Tests  (test full request->response flows)
# ===========================================================================

add_kanba_test(test_integration_auth
    integration/test_auth_flow.cpp
    ${SRC_DIR}/controllers/AuthController.cpp
    ${SRC_DIR}/controllers/HealthController.cpp
    ${SRC_DIR}/filters/AuthFilter.cpp
    ${SRC_DIR}/filters/CorsFilter.cpp
    ${SRC_DIR}/utils/PasswordHash.cpp
    ${SRC_DIR}/utils/Session.cpp
    ${SRC_DIR}/utils/Database.cpp
)

add_kanba_test(test_integration_project_workflow
    integration/test_project_workflow.cpp
    ${SRC_DIR}/controllers/ProjectController.cpp
    ${SRC_DIR}/controllers/ColumnController.cpp
    ${SRC_DIR}/controllers/TaskController.cpp
    ${SRC_DIR}/filters/AuthFilter.cpp
    ${SRC_DIR}/utils/Session.cpp
    ${SRC_DIR}/utils/Database.cpp
)

# ===========================================================================
# Convenience target to run all tests
# ===========================================================================
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS
        test_password_hash test_session test_database
        test_cors_filter test_auth_filter
        test_health_controller test_auth_controller
        test_project_controller test_column_controller
        test_task_controller test_ai_chat_controller
        test_integration_auth test_integration_project_workflow
    COMMENT "Running all kanba backend tests"
)
