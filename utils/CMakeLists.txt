if(NOT DEFINED SYCL_INCLUDE)
    message(FATAL_ERROR "Path to SYCL include didn't provide.")
endif()

file(GLOB UTILS ./*.cpp)
add_library(xpti_utils SHARED ${UTILS})

target_include_directories(xpti_utils PRIVATE ${XPTI_INCLUDE})
target_include_directories(xpti_utils PRIVATE ${SYCL_INCLUDE})

add_library(xptifw SHARED IMPORTED)
set_property(TARGET xptifw PROPERTY IMPORTED_LOCATION ${XPTIFW_DISPATHER})
target_link_libraries(xpti_utils xptifw)
