cmake_minimum_required(VERSION 3.10)
project(
    stim
    VERSION 0.0.0
    DESCRIPTION "Cargo-like build system for C"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(TARGET stim)
set(SOURCE_DIRECTORY "${CMAKE_SOURCE_DIR}/src")
set(VENDOR_DIRECTORY "${CMAKE_SOURCE_DIR}/vendor")

add_executable("${TARGET}"
    "${SOURCE_DIRECTORY}/init.c"
    "${SOURCE_DIRECTORY}/run.c"
    "${SOURCE_DIRECTORY}/stim.c"
    "${SOURCE_DIRECTORY}/util.c"
    "${SOURCE_DIRECTORY}/workspace.c"
)

if (CMAKE_BUILD_TYPE EQUAL "Debug")
    target_compile_options("${TARGET}" -g)
elseif (CMAKE_BUILD_TYPE EQUAL "Release")
    target_compile_options("${TARGET}" -O1)
elseif (CMAKE_BUILD_TYPE EQUAL "RelWithDebInfo")
    target_compile_options("${TARGET}" -g -O1)
elseif (CMAKE_BUILD_TYPE EQUAL "MinSizeRel")
    target_compile_options("${TARGET}" -Oz)
endif()
if (CMAKE_BUILD_TYPE EQUAL "Debug")
    target_compile_options("${TARGET}" -g)
elseif (CMAKE_BUILD_TYPE EQUAL "Release")
    target_compile_options("${TARGET}" -O1)
elseif (CMAKE_BUILD_TYPE EQUAL "RelWithDebInfo")
    target_compile_options("${TARGET}" -g -O1)
elseif (CMAKE_BUILD_TYPE EQUAL "MinSizeRel")
    target_compile_options("${TARGET}" -Oz)
endif()

add_subdirectory("${VENDOR_DIRECTORY}/cwalk")
target_link_libraries("${TARGET}" PRIVATE cwalk)

add_subdirectory("${VENDOR_DIRECTORY}/stb_ds")
target_link_libraries("${TARGET}" PRIVATE stb_ds)

# set(TESTS_DIRECTORY "${CMAKE_SOURCE_DIR}/tests")
# include(CTest)

# if (BUILD_TESTING)
#     add_subdirectory("${VENDOR_DIRECTORY}/Unity")
#     add_subdirectory("${TESTS_DIRECTORY}/Unity")
# endif()