file(GLOB INCLUDE_PROFILER ./include/*.h)
file(GLOB SOURCE_PROFILER ./src/*.cpp)
add_library(profiler_lib SHARED ${INCLUDE_PROFILER} ${SOURCE_PROFILER})

target_include_directories(profiler_lib PRIVATE ./include)
target_include_directories(profiler_lib PRIVATE ${XPTI_INCLUDE})

add_library(xptifw SHARED IMPORTED)
set_property(TARGET xptifw PROPERTY IMPORTED_LOCATION ${XPTIFW_DISPATHER})
target_link_libraries(profiler_lib xptifw)

add_executable(profiler ./main.cpp)
