CPMAddPackage(
  NAME pybind11
  GITHUB_REPOSITORY pybind/pybind11
  VERSION 2.12.0
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../src/sources.cmake)

pybind11_add_module(go_data_gen_python 
    bindings.cpp
    ${GDG_SOURCES}
)

# Set the output name to go_data_gen
set_target_properties(go_data_gen_python PROPERTIES OUTPUT_NAME go_data_gen)

target_link_libraries(go_data_gen_python PRIVATE go_data_gen)
target_include_directories(go_data_gen_python PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

install(TARGETS go_data_gen_python
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
