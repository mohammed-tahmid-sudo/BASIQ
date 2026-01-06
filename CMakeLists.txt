cmake_minimum_required(VERSION 3.10)
project(BASIQ VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Source files
file(GLOB SOURCES "src/*.cpp")
list(APPEND SOURCES "src/codegen.cpp")

# Create executable
add_executable(BASIQ ${SOURCES})

# Include headers
target_include_directories(BASIQ PRIVATE "${PROJECT_SOURCE_DIR}/include")

# LLVM integration
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} at ${LLVM_DIR}")

# Get compile flags
execute_process(
    COMMAND llvm-config --cxxflags
    OUTPUT_VARIABLE LLVM_CXX_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get link flags
execute_process(
    COMMAND llvm-config --ldflags
    OUTPUT_VARIABLE LLVM_LD_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get libraries
execute_process(
    COMMAND llvm-config --libs all
    OUTPUT_VARIABLE LLVM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get system libraries
execute_process(
    COMMAND llvm-config --system-libs
    OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Add flags and libraries to target
target_compile_options(BASIQ PRIVATE ${LLVM_CXX_FLAGS})
target_link_libraries(BASIQ PRIVATE ${LLVM_LD_FLAGS} ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS})


# Output directory
set_target_properties(BASIQ PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Custom run target
add_custom_target(run
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target BASIQ
    COMMAND ${CMAKE_BINARY_DIR}/bin/BASIQ
    DEPENDS BASIQ
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building and running BASIQ..."
)
