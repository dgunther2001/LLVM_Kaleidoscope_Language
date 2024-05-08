project(llvm_kaleidoscope)

set(EXECUTABLE_OUTPUT_PATH ../)

add_executable(test test.cpp)

target_link_libraries(test PRIVATE ) # ADD LIBRARIES LATER