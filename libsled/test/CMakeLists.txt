
include_directories("../src")
add_executable(sled-test sled-test.cc)
target_link_libraries(sled-test sled event pcan)

