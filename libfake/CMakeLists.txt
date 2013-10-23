
include(../Version.cmake)

# Sources
set(Source_Files sled.cc sled_profile.cc)

# Include files to install
set(Include_Files sled.h sled_profile.h)


############################
##  End of configuration  ##
############################


# Generate both static and shared libraries
add_library(${Name_Libfake} STATIC ${Source_Files})
add_library(${Name_Libfake}_shared SHARED ${Source_Files})

# Set verion and install
set(Version ${Version_Major}.${Version_Minor}.${Version_Rel})
set_target_properties(${Name_Libfake}_shared PROPERTIES SOVERSION ${Version_Major})
set_target_properties(${Name_Libfake}_shared PROPERTIES VERSION ${Version})

install(TARGETS ${Name_Libfake} ARCHIVE DESTINATION lib)
install(TARGETS ${Name_Libfake}_shared LIBRARY DESTINATION lib)

# Install include files
install(FILES ${Include_Files} DESTINATION "include/libfake")

