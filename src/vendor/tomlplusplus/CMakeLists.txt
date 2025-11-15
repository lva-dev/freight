project(tomlplusplus)

add_library(tomlplusplus_tomlplusplus INTERFACE)
add_library(tomlplusplus::tomlplusplus ALIAS tomlplusplus_tomlplusplus)

target_include_directories(tomlplusplus_tomlplusplus INTERFACE "${PROJECT_SOURCE_DIR}/include")