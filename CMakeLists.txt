# cmake -DLLVM_DIR=/usr/local/Cellar/llvm/18.1.5/lib/cmake/llvm ..   

cmake_minimum_required(VERSION 3.15)
project(llvm_kaleidoscope)

set(CMAKE_CXX_STANDARD 17)

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(LLVM_LIBS core orcjit X86CodeGen X86AsmParser X86Desc X86Info)

add_subdirectory(include)
add_subdirectory(src)

add_executable(main src/main.cpp src/parser.cpp src/lexer.cpp src/AST.cpp src/codegen.cpp)

target_link_libraries(main ${LLVM_LIBS})

# Option to build examples
# option(BUILD_EXAMPLES "Build example files" ON)

# if (BUILD_EXAMPLES)
#     message(STATUS "Building examples...")
#     add_subdirectory(tests)
# endif()