cmake_minimum_required(VERSION 2.6)

include(../Version.cmake)
include(../Common.cmake)

# Sources
set(Source_Files server.cc rtc3d.cc rtc3d_dataframe.cc)

# Include files
set(Include_Files rtc3d.h rtc3d_dataframe.h)


############################
##  End of configuration  ##
############################


# Both static and shared library
add_library(${Name_Librtc3d} STATIC ${Source_Files})
add_library(${Name_Librtc3d}_shared SHARED ${Source_Files})

target_link_libraries(${Name_Librtc3d}_shared ${LibEvent_Library})
include_directories(${LibEvent_Include})

# Set verion and install
set(Version ${Version_Major}.${Version_Minor}.${Version_Rel})
set_target_properties(${Name_Librtc3d}_shared PROPERTIES SOVERSION ${Version_Major})
set_target_properties(${Name_Librtc3d}_shared PROPERTIES VERSION ${Version})

install(TARGETS ${Name_Librtc3d} ARCHIVE DESTINATION lib)
install(TARGETS ${Name_Librtc3d}_shared LIBRARY DESTINATION lib)

# Install include files
install(FILES ${Include_Files} DESTINATION "include/librtc3d")

