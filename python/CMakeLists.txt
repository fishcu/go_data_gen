find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(gdg_bindings bindings.cpp)
target_link_libraries(gdg_bindings PRIVATE go_data_gen)

install(TARGETS gdg_bindings DESTINATION .)
