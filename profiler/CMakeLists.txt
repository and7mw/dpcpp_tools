if(NOT DEFINED SYCL_INCLUDE)
    message(FATAL_ERROR "Path to SYCL include didn't provide.")
endif()

file(GLOB INCLUDE_PROFILER ./include/*.h)
file(GLOB SOURCE_PROFILER ./src/*.cpp)
add_library(profiler_lib SHARED ${INCLUDE_PROFILER} ${SOURCE_PROFILER})

target_include_directories(profiler_lib PRIVATE ./include)
target_include_directories(profiler_lib PRIVATE ${XPTI_INCLUDE})
target_include_directories(profiler_lib PRIVATE ../utils)
target_include_directories(profiler_lib PRIVATE ${SYCL_INCLUDE})

add_library(xptifw SHARED IMPORTED)
set_property(TARGET xptifw PROPERTY IMPORTED_LOCATION ${XPTIFW_DISPATHER})
target_link_libraries(profiler_lib xptifw)

target_link_libraries(profiler_lib xpti_utils_lib)

add_executable(profiler ./main.cpp)
