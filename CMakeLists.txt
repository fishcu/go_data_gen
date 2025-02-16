cmake_minimum_required(VERSION 3.14)

project(go_data_gen)

include(cmake/get_cpm.cmake)

add_subdirectory(src)

find_package(Python COMPONENTS Interpreter Development REQUIRED)
CPMAddPackage(
  NAME pybind11
  GITHUB_REPOSITORY pybind/pybind11
  VERSION 2.12.0
)
add_subdirectory(python)

add_subdirectory(examples)
