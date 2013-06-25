
CPP=g++
CC=gcc

SRC = sled.cc sled_profile.cc
OBJ = $(patsubst %.cc,.obj/%.o,$(SRC))

.obj:
	mkdir .obj

.obj/%.o: %.cc .obj
	$(CPP) -c -g -fPIC -o $@ $<

.obj/%.o: %.c .obj
	$(CC) -c -g -fPIC -o $@ $<

libsled.so: $(OBJ)
	$(CC) -shared -Wl,-soname,libsled -o ../libsled.so $(OBJ)
