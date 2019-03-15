

add_executable(xcoroutine xcoroutine/main.cpp)

add_executable(xhttp_server xhttp_server/main.cpp)
target_link_libraries(xhttp_server pthread)

add_executable(xhttp_client xhttp_client/main.cpp)
target_link_libraries(xhttp_client pthread)

add_executable(xjson xjson/unit_test.cpp)
