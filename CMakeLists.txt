cmake_minimum_required(VERSION 3.14)

include(cmake/get_cpm.cmake)

project(go_data_gen)

add_subdirectory(src)
add_subdirectory(python)

add_subdirectory(examples)
