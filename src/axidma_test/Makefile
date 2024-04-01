
CC       ?= gcc
CXX      ?= g++
LANGFLAG = -x c++
CPPFLAGS += -Iinclude/ -I/lib/modules/5.10.0-xilinx-v2021.1/extra/dmaproxy/ -Iusr/
CFLAGS   += -g -Wall -O2
LDFLAGS  += $(LIBS) -lpthread -lz -rdynamic -pthread
BUILD_DIR = build
XIL_DIR = lib/modules/5.10.0-xilinx-v2021.1/extra/dmaproxy

ifeq ($(zstd),1)
LDFLAGS		+= -lzstd
endif

BINARY = dma-proxy-test
OBJ = $(BUILD_DIR)/dma-proxy-test.o \

PREFIX = /usr/local
VERSION = `git describe --tags`

ifdef asan
	CFLAGS += -fsanitize=address -fno-omit-frame-pointer
	LDFLAGS += -fsanitize=address -fno-omit-frame-pointer
endif

.PHONY: clean distclean test

$(BINARY): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BUILD_DIR)/dma-proxy-test.o: dma-proxy-test.c dma-proxy.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -c -o $@ $< 

clean:
	rm -rf $(BINARY) $(BUILD_DIR)/*.o

# Delete all gitignored files (but not directories)
distclean: clean
	git clean -f -X
	rm -rf $(BUILD_DIR)/* autom4te.cache
