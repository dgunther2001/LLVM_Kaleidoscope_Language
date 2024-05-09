cmake_minimum_required(VERSION 3.15)
project(llvm_kaleidoscope)

set(CMAKE_CXX_STANDARD 17)


add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(external_libs)

add_executable(main src/main.cpp src/parser.cpp src/lexer.cpp src/AST.cpp)

option(BUILD_EXAMPLES "Test files option active..." ON)

set(BUILD_EXAMPLES OFF) # DISABLE THIS TO TEST WITHIN THE PROJECT

if (BUILD_EXAMPLES)
    message(STATUS "Building tests...")
    add_subdirectory(tests)
endif()