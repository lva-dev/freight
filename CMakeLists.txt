cmake_minimum_required(VERSION 3.10)
project(
    stim
    VERSION 0.0.0
    DESCRIPTION "Cargo-like build system for C"
    LANGUAGES CXX
)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET stim)
set(SOURCE_DIRECTORY "${CMAKE_SOURCE_DIR}/src")
set(VENDOR_DIRECTORY "${CMAKE_SOURCE_DIR}/vendor")

add_executable("${TARGET}"
    "${SOURCE_DIRECTORY}/init.cpp"
    "${SOURCE_DIRECTORY}/run.cpp"
    "${SOURCE_DIRECTORY}/stim.cpp"
    "${SOURCE_DIRECTORY}/util.cpp"
    "${SOURCE_DIRECTORY}/workspace.cpp"
)

if (CMAKE_BUILD_TYPE EQUAL "Debug")
    target_compile_options("${TARGET}" -g)
elseif (CMAKE_BUILD_TYPE EQUAL "Release")
    target_compile_options("${TARGET}" -O3)
elseif (CMAKE_BUILD_TYPE EQUAL "RelWithDebInfo")
    target_compile_options("${TARGET}" -g -O3)
elseif (CMAKE_BUILD_TYPE EQUAL "MinSizeRel")
    target_compile_options("${TARGET}" -Oz)
endif()

# set(TESTS_DIRECTORY "${CMAKE_SOURCE_DIR}/tests")
# include(CTest)

# if (BUILD_TESTING)
#     add_subdirectory("${VENDOR_DIRECTORY}/Unity")
#     add_subdirectory("${TESTS_DIRECTORY}/Unity")
# endif()