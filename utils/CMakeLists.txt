file(GLOB UTILS ./*.cpp)
add_library(xpti_utils_lib SHARED ${UTILS})

target_include_directories(xpti_utils_lib PRIVATE ${XPTI_INCLUDE})

add_library(xptifw SHARED IMPORTED)
set_property(TARGET xptifw PROPERTY IMPORTED_LOCATION ${XPTIFW_DISPATHER})
target_link_libraries(xpti_utils_lib xptifw)
