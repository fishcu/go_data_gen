CPMAddPackage(
  NAME pybind11
  GITHUB_REPOSITORY pybind/pybind11
  VERSION 2.12.0
)

pybind11_add_module(go_data_gen_python 
    bindings.cpp
)

# ${TORCH_LIBRARIES} doesn't include torch_python for some reason, so we need to explictly link it.
find_library(TORCH_PYTHON_LIBRARY torch_python PATH "${TORCH_INSTALL_PREFIX}/lib")

# Set the output name to go_data_gen
set_target_properties(go_data_gen_python PROPERTIES OUTPUT_NAME go_data_gen)
target_link_libraries(go_data_gen_python PRIVATE
  go_data_gen
  ${TORCH_PYTHON_LIBRARY}
  # ${PYTHON_LIBRARIES}
)
target_include_directories(go_data_gen_python PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set_property(TARGET go_data_gen_python PROPERTY POSITION_INDEPENDENT_CODE ON)

install(TARGETS go_data_gen_python
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
