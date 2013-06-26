
SUBDIRS = src

LIB = librtc3d-server.so
INC = src/rtc3d.h src/rtc3d_dataframe.h

PREFIX=/usr/local

LIBRARY_DIR=$(PREFIX)/lib
INCLUDE_DIR=$(PREFIX)/include/librtc3dserver

subdirs:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done

install:
	@echo "Installing to $(LIBRARY_DIR) and $(INCLUDE_DIR)"
	@cp $(LIB) $(LIBRARY_DIR)
	@mkdir -p $(INCLUDE_DIR)
	@cp $(INC) $(INCLUDE_DIR)
	@ldconfig

