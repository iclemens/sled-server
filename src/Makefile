
CPP=g++
CC=gcc

SRC = rtc3d.cc server.cc rtc3d_dataframe.cc
OBJ = $(patsubst %.cc,.obj/%.o,$(SRC))

.obj:
	mkdir .obj

.obj/%.o: %.cc .obj
	$(CPP) -c -g -fPIC -o $@ $<

librtc3d-server.so: $(OBJ)
	$(CC) -shared -Wl,-soname,librtc3d-server -o ../librtc3d-server.so $(OBJ)

