file(GLOB libInclude ./include/*.h)
file(GLOB libSrc ./src/*.cpp)

include_directories(./include)
include_directories(./xpti)

add_library(graph_lib SHARED ${libInclude} ${libSrc})

add_executable(exec_graph_tool ./tools/main.cpp)

target_link_libraries(exec_graph_tool graph_lib)
