project(Profiling_tools_for_DPC++ CXX)
cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 17)

if(NOT DEFINED ENABLE_DUMP_EXEC_GRAPH)
    set(ENABLE_DUMP_EXEC_GRAPH ON)
endif()

if (ENABLE_DUMP_EXEC_GRAPH)
    add_subdirectory(dump_exec_graph)
endif()

# TODO: remove!
add_subdirectory(debug_tool)
