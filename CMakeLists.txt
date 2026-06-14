<<<<<<< HEAD
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
set(SOURCE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(VENDOR_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vendor")

add_executable("${TARGET}"
    "${SOURCE_DIRECTORY}/Init.cpp"
    "${SOURCE_DIRECTORY}/Run.cpp"
    "${SOURCE_DIRECTORY}/Stim.cpp"
    "${SOURCE_DIRECTORY}/Toml.cpp"
    "${SOURCE_DIRECTORY}/Util.cpp"
    "${SOURCE_DIRECTORY}/Workspace.cpp"
)

if (CMAKE_BUILD_TYPE EQUAL "Debug")
    target_compile_options("${TARGET}" -g)
elseif (CMAKE_BUILD_TYPE EQUAL "Release")
    target_compile_definitions("${TARGET}" -NDEBUG)
    target_compile_options("${TARGET}" -O3)
elseif (CMAKE_BUILD_TYPE EQUAL "RelWithDebInfo")
    target_compile_definitions("${TARGET}" -NDEBUG)
    target_compile_options("${TARGET}" -g -O3)
elseif (CMAKE_BUILD_TYPE EQUAL "MinSizeRel")
    target_compile_definitions("${TARGET}" -NDEBUG)
    target_compile_options("${TARGET}" -Oz)
endif()

target_include_directories("${TARGET}" PRIVATE "${VENDOR_DIRECTORY}/marzer/include/")

# set(TESTS_DIRECTORY "${CMAKE_SOURCE_DIR}/tests")
# include(CTest)
=======
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
>>>>>>> old/main
