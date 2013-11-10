cmake_minimum_required(VERSION 2.6)

include(../Version.cmake)
include(../Common.cmake)

# Sources
set(Source_Files sled.cc sled_profile.cc interface.cc 
  machines/mch_intf.cc machines/mch_net.cc 
  machines/mch_sdo.cc machines/mch_ds.cc machines/mch_mp.cc)

# Include files to install
set(Include_Files sled.h sled_profile.h)


############################
##  End of configuration  ##
############################

find_path(LibPcan_Include libpcan.h)
include_directories(${LibPcan_Include})

# Generate both static and shared libraries
add_library(${Name_Libsled} STATIC ${Source_Files})
add_library(${Name_Libsled}_shared SHARED ${Source_Files})

target_link_libraries(${Name_Libsled}_shared ${LibEvent_Library})
include_directories(${LibEvent_Include})

# Set verion and install
set(Version ${Version_Major}.${Version_Minor}.${Version_Rel})
set_target_properties(${Name_Libsled}_shared PROPERTIES SOVERSION ${Version_Major})
set_target_properties(${Name_Libsled}_shared PROPERTIES VERSION ${Version})

install(TARGETS ${Name_Libsled} ARCHIVE DESTINATION lib)
install(TARGETS ${Name_Libsled}_shared LIBRARY DESTINATION lib)

# Install include files
install(FILES ${Include_Files} DESTINATION "include/libsled")

