include(sources.cmake)

add_library(go_data_gen ${GDG_SOURCES})
set_property(TARGET go_data_gen PROPERTY CXX_STANDARD 17)
target_include_directories(go_data_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set_property(TARGET go_data_gen PROPERTY POSITION_INDEPENDENT_CODE ON)
