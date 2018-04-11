add_executable(echo_server EchoServer.cc)
target_link_libraries(echo_server tinyev)

add_executable(echo_client EchoClient.cc)
target_link_libraries(echo_client tinyev)

add_executable(discard_server DiscardServer.cc)
target_link_libraries(discard_server tinyev)

add_executable(chargen_server ChargenServer.cc)
target_link_libraries(chargen_server tinyev)

add_executable(timer TimerLoop.cc)
target_link_libraries(timer tinyev)

#add_subdirectory(echo_bench)
add_subdirectory(nqueen)
add_subdirectory(kth_element)