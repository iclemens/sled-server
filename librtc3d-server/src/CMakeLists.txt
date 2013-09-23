
# Version information
set(Version_Major 1)
set(Version_Minor 0)
set(Version_Rel 0)

# Sources
set(Source_Files server.cc rtc3d.cc rtc3d_dataframe.cc)

# Include files
set(Include_Files rtc3d.h rtc3d_dataframe.h)


############################
##  End of configuration  ##
############################


# Both static and shared library
add_library(rtc3d-server_static STATIC ${Source_Files})
add_library(rtc3d-server SHARED ${Source_Files})

# Set verion and install
set(Version ${Version_Major}.${Version_Minor}.${Version_Rel})
set_target_properties(rtc3d-server PROPERTIES SOVERSION ${Version_Major})
set_target_properties(rtc3d-server PROPERTIES VERSION ${Version})

install(TARGETS rtc3d-server_static ARCHIVE DESTINATION lib)
install(TARGETS rtc3d-server LIBRARY DESTINATION lib)

# Install include files
install(FILES ${Include_Files} DESTINATION "include/librtc3d-server")

