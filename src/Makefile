
CPP=g++
CC=gcc

YAC = parser.yac
LEX = parser.lex
SRC = main.cc server.cc

OBJ = $(patsubst %.cc,.obj/%.o,$(SRC))
OBJ += $(patsubst %.yac,.obj/%.yac.o,$(YAC))
OBJ += $(patsubst %.lex,.obj/%.lex.o,$(LEX))

all: server

.gen:
	mkdir .gen

.obj:
	mkdir .obj

.gen/parser.yac.cc: parser.yac .gen
	bison -o $@ -d $<

.gen/parser.lex.cc: parser.lex .gen
	flex -o $@ $<

.obj/%.o: .gen/%.cc
	$(CC) -c -g -fPIC -o $@ $< -I.

.obj/%.o: %.cc
	$(CC) -c -g -fPIC -o $@ $<

server: $(OBJ)
	$(CPP) -rdynamic -o ../sled_server $(OBJ) -levent -lsled -lpcan -lrtc3d-server

