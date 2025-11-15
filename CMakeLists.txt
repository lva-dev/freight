cmake_minimum_required(VERSION 3.15)
project(freight)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET ${PROJECT_NAME})

add_executable(${TARGET}
    src/main.cpp
    src/new.cpp
    src/build.cpp
    src/compiler.cpp
)

target_compile_options(${TARGET} PRIVATE
    -Wall
    -Wextra
    -pedantic
    -g
)

target_link_options(${TARGET} PRIVATE
    -fsanitize=address
)

add_subdirectory(src/vendor/tomlplusplus)
target_link_libraries(${TARGET} PRIVATE tomlplusplus::tomlplusplus)
