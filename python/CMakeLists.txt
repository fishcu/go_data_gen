CPMAddPackage(
  NAME pybind11
  GITHUB_REPOSITORY pybind/pybind11
  VERSION 2.12.0
)

pybind11_add_module(go_data_gen_python 
    bindings.cpp
    ${GDG_SOURCES}
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../src/sources.cmake)
target_link_libraries(go_data_gen_python PRIVATE go_data_gen)
target_include_directories(go_data_gen_python PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
