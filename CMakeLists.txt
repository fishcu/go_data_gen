cmake_minimum_required(VERSION 3.14)

include(cmake/get_cpm.cmake)

project(go_data_gen)

# find_package(Torch REQUIRED PATHS "${CMAKE_SOURCE_DIR}/libtorch")
find_package(Torch REQUIRED PATHS "${CMAKE_SOURCE_DIR}/libtorch_self_built")

add_subdirectory(src)
add_subdirectory(python)

add_subdirectory(examples)
