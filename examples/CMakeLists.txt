add_executable(play_sgf play_sgf.cpp)
target_link_libraries(play_sgf PRIVATE go_data_gen)
set_property(TARGET play_sgf PROPERTY CXX_STANDARD 17)
