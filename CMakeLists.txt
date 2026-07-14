cmake_minimum_required(VERSION 3.14)
project(
    freight
    VERSION 0.1.0
    DESCRIPTION "Cargo-like build system for C"
    LANGUAGES CXX
)

include(FetchContent)

FetchContent_Declare(GSL
    GIT_REPOSITORY "https://github.com/microsoft/GSL"
    GIT_TAG "v4.2.0"
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(GSL)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET freight)
set(SOURCE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(VENDOR_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vendor")

add_executable("${TARGET}"
    "${SOURCE_DIRECTORY}/Init.cpp"
    "${SOURCE_DIRECTORY}/Io.cpp"
    "${SOURCE_DIRECTORY}/Run.cpp"
    "${SOURCE_DIRECTORY}/Main.cpp"
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

target_link_libraries("${TARGET}" PRIVATE Microsoft.GSL::GSL)

# set(TESTS_DIRECTORY "${CMAKE_SOURCE_DIR}/tests")
# include(CTest)