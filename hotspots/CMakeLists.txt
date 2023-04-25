if(NOT DEFINED SYCL_INCLUDE)
    message(FATAL_ERROR "Path to SYCL include didn't provide.")
endif()

file(GLOB INCLUDE_PROFILER ./include/*.h)
file(GLOB SOURCE_PROFILER ./src/*.cpp)
add_library(hotspots_sub SHARED ${INCLUDE_PROFILER} ${SOURCE_PROFILER})

target_include_directories(hotspots_sub PRIVATE ./include)
target_include_directories(hotspots_sub PRIVATE ${XPTI_INCLUDE})
target_include_directories(hotspots_sub PRIVATE ../utils)
target_include_directories(hotspots_sub PRIVATE ${SYCL_INCLUDE})

add_library(xptifw SHARED IMPORTED)
set_property(TARGET xptifw PROPERTY IMPORTED_LOCATION ${XPTIFW_DISPATHER})
target_link_libraries(hotspots_sub xptifw)

target_link_libraries(hotspots_sub xpti_utils)

add_executable(hotspots ./main.cpp)
