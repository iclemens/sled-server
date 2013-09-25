
include(../Version.cmake)

# Source files and executable name
set(Source_Files main.cc server.cc)
set(Executable_Name ${Name_Executable})

# Compile Bison and Flex into source code
find_package(BISON)
find_package(FLEX)

bison_target(parser parser.yac ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)
flex_target(scanner scanner.lex ${CMAKE_CURRENT_BINARY_DIR}/scanner.cc)

# Create executable
add_executable(${Executable_Name} ${Source_Files} 
  ${CMAKE_CURRENT_BINARY_DIR}/scanner.cc 
  ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(${Name_Executable} ${Name_Libsled} ${Name_Librtc3d} pcan event)

# Install executable
install(TARGETS ${Name_Executable} RUNTIME DESTINATION bin)
