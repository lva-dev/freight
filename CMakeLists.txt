cmake_minimum_required(VERSION 3.15)
project(freight)

if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET freight)
set(SRC src)

add_executable(${TARGET}
    "${SRC}/build.cpp"
    "${SRC}/compiler.cpp"
    "${SRC}/cli.cpp"
    "${SRC}/main.cpp"
    "${SRC}/new.cpp"
    "${SRC}/run.cpp"
)

target_precompile_headers(${TARGET} PRIVATE
    "${SRC}/pch.h"
)

target_compile_options(${TARGET} PRIVATE
    -Wall -Wextra -pedantic
)

if(CMAKE_BUILD_TYPE IN_LIST "Debug;RelWithDebInfo")
    target_compile_options(${TARGET} PRIVATE
        -g
    )

    target_link_options(${TARGET} PRIVATE
        -fsanitize=address
        -fsanitize=undefined
    )
endif()

set(VENDOR "${SRC}/vendor")

add_subdirectory("${VENDOR}/tomlplusplus")
target_link_libraries(${TARGET} PRIVATE tomlplusplus)

add_subdirectory("${VENDOR}/CLI11")
target_link_libraries(${TARGET} PRIVATE CLI11)

add_subdirectory(tests)