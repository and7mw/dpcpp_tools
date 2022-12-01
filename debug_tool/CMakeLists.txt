file(GLOB SOURCE_DEBUG_TOOL ./src/*.cpp)
add_library(debug_tool_lib SHARED ${SOURCE_DEBUG_TOOL})

target_include_directories(debug_tool_lib PRIVATE ${XPTI_INCLUDE})
target_include_directories(debug_tool_lib PRIVATE "/home/maxim/master_science_work/llvm/sycl/include")

add_library(xptifw SHARED IMPORTED)
set_property(TARGET xptifw PROPERTY IMPORTED_LOCATION ${XPTIFW_DISPATHER})
target_link_libraries(debug_tool_lib xptifw)
