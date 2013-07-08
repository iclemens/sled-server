
SUBDIRS = src

BIN = sled_server

PREFIX=/usr/local

BINARY_DIR=$(PREFIX)/bin

subdirs:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done

install:
	@echo "Installing to $(BINARY_DIR)"
	@cp $(BIN) $(BINARY_DIR)

