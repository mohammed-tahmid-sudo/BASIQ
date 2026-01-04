cmake_minimum_required(VERSION 3.10)
project(MyCompiler VERSION 0.1 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(include)

# Source files
file(GLOB SOURCES "src/*.cpp")

# Create executable
add_executable(my_compiler ${SOURCES})

# Optionally, set output directory
set_target_properties(my_compiler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)


# Add a custom "run" target
add_custom_target(run
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target my_compiler
    COMMAND ${CMAKE_BINARY_DIR}/bin/my_compiler
    DEPENDS my_compiler
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building and running my_compiler..."
)
