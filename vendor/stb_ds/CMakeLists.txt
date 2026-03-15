cmake_minimum_required(VERSION 3.10)
project(stb_ds)

add_library(stb_ds src/stb_ds.c)
target_include_directories(stb_ds PUBLIC include)