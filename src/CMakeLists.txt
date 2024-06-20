set(GDG_SOURCES
  board.cpp
  sgf.cpp
)

add_library(go_data_gen ${GDG_SOURCES})
target_include_directories(go_data_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

include(../cmake/CPM.cmake)
# CPMAddPackage(
#   NAME libsgfcplusplus
#   GITHUB_REPOSITORY herzbube/libsgfcplusplus
#   GIT_TAG master  
# )
# target_link_libraries(go_data_gen PUBLIC libsgfcplusplus)
