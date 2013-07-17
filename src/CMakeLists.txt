
# Set program version
set(Version_Major 1)
set(Version_Minor 0)
set(Version_Rel 0)

# Source files and executable name
set(Source_Files main.cc server.cc)
set(Executable_Name 
  sled-server-${Version_Major}.${Version_Minor})

# Compile Bison and Flex into source code
find_package(BISON)
find_package(FLEX)

bison_target(parser parser.yac ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)
flex_target(scanner scanner.lex ${CMAKE_CURRENT_BINARY_DIR}/scanner.cc)

# Create executable
add_executable(${Executable_Name} ${Source_Files} 
  ${CMAKE_CURRENT_BINARY_DIR}/scanner.cc 
  ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${Executable_Name} sled rtc3d-server pcan event)

