add_executable(kth_server KthServer.cc Codec.cc Codec.h)
target_link_libraries(kth_server tinyev)

add_executable(kth_client KthClient.cc Codec.cc Codec.h)
target_link_libraries(kth_client tinyev)