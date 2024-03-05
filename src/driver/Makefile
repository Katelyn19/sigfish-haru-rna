CC       ?= gcc
CXX      ?= g++
LANGFLAG = -x c++
CPPFLAGS += -Iinclude/
CFLAGS   += -g -Wall -O2 -std=c++11
LDFLAGS  += $(LIBS) -lpthread -lz -rdynamic
BUILD_DIR = build

ifeq ($(zstd),1)
LDFLAGS		+= -lzstd
endif

BINARY = fifo_test
OBJ = $(BUILD_DIR)/fifo_test.o \
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

$(BUILD_DIR)/main.o: src/main.c include/fifo_test.h
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(LANGFLAG) $< -c -o $@

#haru things
$(BUILD_DIR)/fifo_test.o: src/fifo_test.c
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(LANGFLAG) $< -c -o $@

clean:
	rm -rf $(BINARY) $(BUILD_DIR)/*.o

# Delete all gitignored files (but not directories)
distclean: clean
	git clean -f -X
	rm -rf $(BUILD_DIR)/* autom4te.cache
