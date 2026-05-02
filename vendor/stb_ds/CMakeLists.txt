cmake_minimum_required(VERSION 3.10)
project(stb_ds)

set(TARGET stb_ds)

add_library("${TARGET}" src/stb_ds.c)
target_include_directories("${TARGET}" PUBLIC include)

if (CMAKE_BUILD_TYPE EQUAL "Debug")
    target_compile_options("${TARGET}" -g)
elseif (CMAKE_BUILD_TYPE EQUAL "Release")
    target_compile_options("${TARGET}" -O3)
elseif (CMAKE_BUILD_TYPE EQUAL "RelWithDebInfo")
    target_compile_options("${TARGET}" -g -O3)
elseif (CMAKE_BUILD_TYPE EQUAL "MinSizeRel")
    target_compile_options("${TARGET}" -Oz)
endif()