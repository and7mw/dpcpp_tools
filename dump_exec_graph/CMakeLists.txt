if(NOT DEFINED SYCL_INCLUDE)
    message(FATAL_ERROR "Path to SYCL include didn't provide.")
endif()

file(GLOB INCLUDE_DUMP_EXEC_GRAPH ./include/*.h)
file(GLOB SOURCE_DUMP_EXEC_GRAPH ./src/*.cpp)
add_library(dump_exec_graph_lib SHARED ${INCLUDE_DUMP_EXEC_GRAPH} ${SOURCE_DUMP_EXEC_GRAPH})

target_include_directories(dump_exec_graph_lib PRIVATE ./include)
target_include_directories(dump_exec_graph_lib PRIVATE ${XPTI_INCLUDE})
target_include_directories(dump_exec_graph_lib PRIVATE ../utils)
target_include_directories(dump_exec_graph_lib PRIVATE ${SYCL_INCLUDE})

add_library(xptifw SHARED IMPORTED)
set_property(TARGET xptifw PROPERTY IMPORTED_LOCATION ${XPTIFW_DISPATHER})
target_link_libraries(dump_exec_graph_lib xptifw)

target_link_libraries(dump_exec_graph_lib xpti_utils_lib)

add_executable(dump_exec_graph ./main.cpp)
