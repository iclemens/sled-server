
set(Version_Branch master)
set(Version_Major 1)
set(Version_Minor 0)
set(Version_Rel 0)

#
# We define names here as they are used in 
# multiple locations.
#
if(Version_Branch STREQUAL master)
  set(Name_Executable sled-server-${Version_Major}.${Version_Minor})
  set(Name_Libsled sled)
  set(Name_Librtc3d rtc3d-server)
else()
  set(Name_Executable sled-server-${Version_Branch}-${Version_Major}.${Version_Minor})
  set(Name_Libsled sled-${Version_Branch})
  set(Name_Librtc3d rtc3d-server-${Version_Branch})
endif()

