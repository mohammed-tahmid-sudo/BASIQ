set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required(VERSION 3.10)
project(BASIQ VERSION 0.1 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Source files
file(GLOB SOURCES "src/*.cpp")

# Create executable
add_executable(BASIQ ${SOURCES})

# Include headers from include/ folder
target_include_directories(BASIQ PRIVATE "${PROJECT_SOURCE_DIR}/include")

# Optionally, set output directory
set_target_properties(BASIQ PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Add a custom "run" target
add_custom_target(run
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target BASIQ
    COMMAND ${CMAKE_BINARY_DIR}/bin/BASIQ
    DEPENDS BASIQ
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building and running BASIQ..."
)


