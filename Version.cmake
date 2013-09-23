

set(Version_Major 1)
set(Version_Minor 1)
set(Version_Rel 0)


##########################
# Extract GIT information


# Name of the current branch
execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE Version_Branch
  OUTPUT_STRIP_TRAILING_WHITESPACE)

# Last commit hash
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE Version_Hash
  OUTPUT_STRIP_TRAILING_WHITESPACE)


#########################
# Make accessible from C


add_definitions("-DVERSION_MAJOR=${Version_Major}")
add_definitions("-DVERSION_MINOR=${Version_Minor}")
add_definitions("-DVERSION_BRANCH=${Version_Branch}")
add_definitions("-DVERSION_HASH=${Version_Hash}")


##################################
# Determine names for executables


# We define names here as they are used in multiple locations.
if(Version_Branch STREQUAL master)
  set(Name_Executable sled-server-${Version_Major}.${Version_Minor})
  set(Name_Libsled sled)
  set(Name_Librtc3d rtc3d)
else()
  set(Name_Executable sled-server-${Version_Branch}-${Version_Major}.${Version_Minor})
  set(Name_Libsled sled-${Version_Branch})
  set(Name_Librtc3d rtc3d-${Version_Branch})
endif()

