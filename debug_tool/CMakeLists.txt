if(NOT DEFINED XPTI_INCLUDE)
    message(FATAL_ERROR "Path to XPTI include didn't provide.")
endif()

if(NOT DEFINED XPTIFW_DISPATHER)
    message(FATAL_ERROR "Path to XPTIFW dispatcher didn't provide.")
endif()

file(GLOB SOURCE_DEBUG_TOOL ./src/*.cpp)
add_library(debug_tool_lib SHARED ${SOURCE_DEBUG_TOOL})

target_include_directories(debug_tool_lib PRIVATE ${XPTI_INCLUDE})

add_library(xptifw SHARED IMPORTED)
set_property(TARGET xptifw PROPERTY IMPORTED_LOCATION ${XPTIFW_DISPATHER})
target_link_libraries(debug_tool_lib xptifw)
