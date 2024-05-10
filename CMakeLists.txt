cmake_minimum_required(VERSION 3.15)
project(llvm_kaleidoscope)

set(CMAKE_CXX_STANDARD 17)



include_directories(${CMAKE_SOURCE_DIR}/external_libs/llvm-project/llvm)

set(LLVM_DIR "${CMAKE_SOURCE_DIR}/external_libs/llvm-project/llvm/build/lib/cmake/llvm")

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(LLVM_LIBS core)

add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(external_libs)

add_executable(main src/main.cpp src/parser.cpp src/lexer.cpp src/ast.cpp src/codegen.cpp)


target_link_libraries(main ${LLVM_LIBS})

option(BUILD_EXAMPLES "Test files option active..." ON)

set(BUILD_EXAMPLES OFF) # DISABLE THIS TO TEST WITHIN THE PROJECT

if (BUILD_EXAMPLES)
    message(STATUS "Building tests...")
    add_subdirectory(tests)
endif()