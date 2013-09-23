
# Version information
set(Version_Major 1)
set(Version_Minor 0)
set(Version_Rel 0)

# Sources
set(Source_Files sled.cc sled_profile.cc interface.cc 
  machines/mch_intf.cc machines/mch_net.cc 
  machines/mch_sdo.cc machines/mch_ds.cc machines/mch_mp.cc)

# Include files to install
set(Include_Files sled.h sled_profile.h)


############################
##  End of configuration  ##
############################


# Generate both static and shared libraries
add_library(sled_static STATIC ${Source_Files})
add_library(sled SHARED ${Source_Files})

# Set verion and install
set(Version ${Version_Major}.${Version_Minor}.${Version_Rel})
set_target_properties(sled PROPERTIES SOVERSION ${Version_Major})
set_target_properties(sled PROPERTIES VERSION ${Version})

install(TARGETS sled_static ARCHIVE DESTINATION lib)
install(TARGETS sled LIBRARY DESTINATION lib)

# Install include files
install(FILES ${Include_Files} DESTINATION "include/libsled")

