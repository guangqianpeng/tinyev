add_executable(echo echo.cc)
target_link_libraries(echo tinyev)

add_executable(discard discard.cc)
target_link_libraries(discard tinyev)

add_executable(chargen chargen.cc ../kth_element/Codec.cc ../kth_element/Codec.h)
target_link_libraries(chargen tinyev)

add_executable(hello hello.cc)
target_link_libraries(hello tinyev)