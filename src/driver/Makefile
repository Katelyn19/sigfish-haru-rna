
CC       ?= gcc
LANGFLAG = -x c
CPPFLAGS += -Iinclude/
CFLAGS   += -g -Wall -O2
LDFLAGS  += $(LIBS)
BUILD_DIR = build


BINARY = mcdma_test
OBJ = $(BUILD_DIR)/mcdma_test.o \
	$(BUILD_DIR)/main.o \

PREFIX = /usr/local
VERSION = `git describe --tags`

ifdef asan
	CFLAGS += -fsanitize=address -fno-omit-frame-pointer
	LDFLAGS += -fsanitize=address -fno-omit-frame-pointer
endif

.PHONY: clean distclean test

$(BINARY): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BUILD_DIR)/main.o: src/main.c include/mcdma_test.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LANGFLAG) $< -c -o $@

#haru things
$(BUILD_DIR)/mcdma_test.o: src/mcdma_test.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LANGFLAG) $< -c -o $@

clean:
	rm -rf $(BINARY) $(BUILD_DIR)/*.o

# Delete all gitignored files (but not directories)
distclean: clean
	git clean -f -X
	rm -rf $(BUILD_DIR)/* autom4te.cache
