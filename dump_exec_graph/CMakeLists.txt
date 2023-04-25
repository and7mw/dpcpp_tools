if(NOT DEFINED SYCL_INCLUDE)
    message(FATAL_ERROR "Path to SYCL include didn't provide.")
endif()

file(GLOB INCLUDE_DUMP_EXEC_GRAPH ./include/*.h)
file(GLOB SOURCE_DUMP_EXEC_GRAPH ./src/*.cpp)
add_library(dump_exec_graph_sub SHARED ${INCLUDE_DUMP_EXEC_GRAPH} ${SOURCE_DUMP_EXEC_GRAPH})

target_include_directories(dump_exec_graph_sub PRIVATE ./include)
target_include_directories(dump_exec_graph_sub PRIVATE ${XPTI_INCLUDE})
target_include_directories(dump_exec_graph_sub PRIVATE ../utils)
target_include_directories(dump_exec_graph_sub PRIVATE ${SYCL_INCLUDE})

add_library(xptifw SHARED IMPORTED)
set_property(TARGET xptifw PROPERTY IMPORTED_LOCATION ${XPTIFW_DISPATHER})
target_link_libraries(dump_exec_graph_sub xptifw)

target_link_libraries(dump_exec_graph_sub xpti_utils)

add_executable(dump_exec_graph ./main.cpp)
