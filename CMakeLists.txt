project(DUMMY CXX)
cmake_minimum_required(VERSION 3.23)

file(GLOB libInclude ./include/*.h)
file(GLOB libSrc ./src/*.cpp)

include_directories(./include)
include_directories(./xpti)

add_library(graph_lib SHARED ${libInclude} ${libSrc})

add_library(xptifw SHARED IMPORTED)
set_property(TARGET xptifw PROPERTY IMPORTED_LOCATION "/home/maxim/master_science_work/llvm/xptifw/lib/Debug/libxptifw.so")
target_link_libraries(graph_lib xptifw)

add_executable(exec_graph_tool ./tools/main.cpp)
target_link_libraries(exec_graph_tool graph_lib)
