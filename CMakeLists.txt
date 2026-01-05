cmake_minimum_required(VERSION 3.10)
project(BASIQ VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source files
file(GLOB SOURCES "src/*.cpp")

# Create executable
add_executable(BASIQ ${SOURCES})

# Include headers
target_include_directories(BASIQ PRIVATE "${PROJECT_SOURCE_DIR}/include")

# LLVM integration

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} at ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(BASIQ PRIVATE ${llvm_libs})

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

