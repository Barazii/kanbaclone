cmake_minimum_required(VERSION 3.16)
project(kanba-backend CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Drogon
find_package(Drogon CONFIG REQUIRED)

# Find libsodium
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

# Find UUID
pkg_check_modules(UUID REQUIRED uuid)

# Source files
set(SOURCES
    src/main.cpp
    src/controllers/AuthController.cpp
    src/controllers/ProjectController.cpp
    src/controllers/ColumnController.cpp
    src/controllers/TaskController.cpp
    src/controllers/AiChatController.cpp
    src/controllers/HealthController.cpp
    src/filters/CorsFilter.cpp
    src/filters/AuthFilter.cpp
    src/utils/PasswordHash.cpp
    src/utils/Session.cpp
    src/utils/Database.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SODIUM_INCLUDE_DIRS}
    ${UUID_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Drogon::Drogon
    ${SODIUM_LIBRARIES}
    ${UUID_LIBRARIES}
)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    ${SODIUM_CFLAGS_OTHER}
    ${UUID_CFLAGS_OTHER}
)

# Copy config file to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.json
               ${CMAKE_CURRENT_BINARY_DIR}/config.json COPYONLY)
