
SUBDIRS = src

LIB = libsled.so
INC = src/sled.h src/sled_profile.h

PREFIX=/usr/local

LIBRARY_DIR=$(PREFIX)/lib
INCLUDE_DIR=$(PREFIX)/include/libsled

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

