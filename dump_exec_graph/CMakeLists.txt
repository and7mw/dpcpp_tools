file(GLOB INCLUDE_DUMP_EXEC_GRAPH ./include/*.h)
file(GLOB SOURCE_DUMP_EXEC_GRAPH ./src/*.cpp)
add_library(dump_exec_graph_lib SHARED ${INCLUDE_DUMP_EXEC_GRAPH} ${SOURCE_DUMP_EXEC_GRAPH})

target_include_directories(dump_exec_graph_lib PRIVATE ./include)
target_include_directories(dump_exec_graph_lib PRIVATE ${XPTI_INCLUDE})

add_library(xptifw SHARED IMPORTED)
set_property(TARGET xptifw PROPERTY IMPORTED_LOCATION ${XPTIFW_DISPATHER})
target_link_libraries(dump_exec_graph_lib xptifw)

add_executable(dump_exec_graph ./main.cpp)
