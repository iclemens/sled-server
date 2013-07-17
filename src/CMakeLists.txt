
set(Version_Major 1)
set(Version_Minor 0)
set(Version_Rel 0)

set(Source_Files main.cc server.cc)

find_package(BISON)
find_package(FLEX)

bison_target(parser parser.yac ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)
flex_target(scanner scanner.lex ${CMAKE_CURRENT_BINARY_DIR}/scanner.cc)

add_executable(sled-server ${Source_Files} 
  ${CMAKE_CURRENT_BINARY_DIR}/scanner.cc 
  ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sled-server sled rtc3d-server pcan event)

