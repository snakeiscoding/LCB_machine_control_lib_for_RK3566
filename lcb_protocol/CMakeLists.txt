cmake_minimum_required(VERSION 3.12)
project(lcb_protocol VERSION 1.0.0 LANGUAGES C)

# Set C standard (gnu11, same as the Makefile)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# 5-layer module layout: inc/<module>/*.h and src/<module>/*.c
set(LCB_MODULES
    protocol/Comm
    protocol/MachineHandler
    adapters
    capabilities
    control
    hardware
)

# Source / header files (every module directory plus bundled cJSON)
set(LCB_SOURCES)
set(LCB_HEADERS)
set(LCB_INCLUDE_DIRS)
foreach(module IN LISTS LCB_MODULES)
    file(GLOB module_sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/${module}/*.c)
    file(GLOB module_headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/inc/${module}/*.h)
    list(APPEND LCB_SOURCES ${module_sources})
    list(APPEND LCB_HEADERS ${module_headers})
    list(APPEND LCB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/inc/${module})
endforeach()
list(APPEND LCB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cJSON/cJSON.c)
list(APPEND LCB_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cJSON/cJSON.h)
list(APPEND LCB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cJSON)

# Public headers for install (internal headers stay private)
set(LCB_PUBLIC_HEADERS ${LCB_HEADERS})
list(FILTER LCB_PUBLIC_HEADERS EXCLUDE REGEX "_internal\\.h$")

find_package(Threads REQUIRED)

# Common settings for the static and shared library targets
function(lcb_protocol_configure target)
    target_include_directories(${target} PUBLIC ${LCB_INCLUDE_DIRS})
    target_compile_options(${target} PRIVATE -Wall -Wextra)
    target_link_libraries(${target} PUBLIC Threads::Threads m)
endfunction()

# Build static library
add_library(lcb_protocol_static STATIC ${LCB_SOURCES} ${LCB_HEADERS})
set_target_properties(lcb_protocol_static PROPERTIES
    OUTPUT_NAME lcb_protocol
    POSITION_INDEPENDENT_CODE ON)
lcb_protocol_configure(lcb_protocol_static)

# Build shared library (optional)
option(BUILD_SHARED_LIBS "Build shared library" OFF)
if(BUILD_SHARED_LIBS)
    add_library(lcb_protocol_shared SHARED ${LCB_SOURCES} ${LCB_HEADERS})
    set_target_properties(lcb_protocol_shared PROPERTIES OUTPUT_NAME lcb_protocol)
    lcb_protocol_configure(lcb_protocol_shared)
endif()

# Build test programs
option(BUILD_TESTS "Build test programs" ON)
if(BUILD_TESTS)
    enable_testing()
    foreach(test_name test_lcb test_constants)
        add_executable(${test_name} test/${test_name}.c)
        target_link_libraries(${test_name} PRIVATE lcb_protocol_static)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

# Build example programs
option(BUILD_EXAMPLES "Build example programs" OFF)
if(BUILD_EXAMPLES)
    add_executable(simple_usage examples/simple_usage.c)
    target_link_libraries(simple_usage PRIVATE lcb_protocol_static)
endif()

# Installation
install(TARGETS lcb_protocol_static
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)
if(BUILD_SHARED_LIBS)
    install(TARGETS lcb_protocol_shared
            LIBRARY DESTINATION lib)
endif()

install(FILES ${LCB_PUBLIC_HEADERS}
        DESTINATION include/lcb_protocol)

# Package information
set(CPACK_PACKAGE_NAME "lcb_protocol")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION "LCB Protocol Communication Library")
include(CPack)
