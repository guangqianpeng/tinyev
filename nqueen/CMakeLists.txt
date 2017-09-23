add_executable(n_queen_server NQueenServer.cc Codec.cc Codec.h)
target_link_libraries(n_queen_server tinyev)

add_executable(n_queen_client NQueenClient.cc Codec.cc Codec.h)
target_link_libraries(n_queen_client tinyev)