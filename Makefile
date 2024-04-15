CC       ?= gcc
CXX      ?= g++
LANGFLAG = -x c++
CPPFLAGS += -I slow5lib/include/
CFLAGS   += -g -Wall -O2  -std=c99
LDFLAGS  += $(LIBS) -lpthread -lz -rdynamic -lm
BUILD_DIR = build

ifeq ($(zstd),1)
LDFLAGS		+= -lzstd
endif

BINARY = sigfish
OBJ = $(BUILD_DIR)/main.o \
      $(BUILD_DIR)/dtw_main.o \
      $(BUILD_DIR)/sigfish.o \
      $(BUILD_DIR)/thread.o \
      $(BUILD_DIR)/events.o \
      $(BUILD_DIR)/model.o \
      $(BUILD_DIR)/cdtw.o \
	  $(BUILD_DIR)/genref.o \
	  $(BUILD_DIR)/jnn.o \
	  $(BUILD_DIR)/misc.o \
	  $(BUILD_DIR)/eval.o \

PREFIX = /usr/local
VERSION = `git describe --tags`

ifdef asan
	CFLAGS += -fsanitize=address -fno-omit-frame-pointer
	LDFLAGS += -fsanitize=address -fno-omit-frame-pointer
endif

ifdef acc
	OBJ +=	$(BUILD_DIR)/haru.o $(BUILD_DIR)/axi_dma.o $(BUILD_DIR)/dtw_accel.o
	CPPFLAGS += -DHAVE_ACC=1 -I HARU-multi-accel/driver/include/
endif
.PHONY: clean distclean test

$(BINARY): $(OBJ) slow5lib/lib/libslow5.a
	$(CC) $(CFLAGS) $(OBJ) slow5lib/lib/libslow5.a $(LDFLAGS) -o $@

$(BUILD_DIR)/main.o: src/main.c src/misc.h src/error.h src/sigfish.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

$(BUILD_DIR)/sigfish.o: src/sigfish.c src/misc.h src/error.h src/sigfish.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

$(BUILD_DIR)/thread.o: src/thread.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

$(BUILD_DIR)/dtw_main.o: src/dtw_main.c src/error.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

$(BUILD_DIR)/events.o: src/events.c src/misc.h src/ksort.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

$(BUILD_DIR)/model.o: src/model.c src/model.h  src/misc.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

$(BUILD_DIR)/cdtw.o: src/cdtw.c src/cdtw.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

$(BUILD_DIR)/genref.o: src/genref.c src/ref.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

$(BUILD_DIR)/jnn.o: src/jnn.c src/jnn.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

$(BUILD_DIR)/misc.o: src/misc.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

$(BUILD_DIR)/eval.o: src/eval.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

#haru things
$(BUILD_DIR)/haru.o: HARU-multi-accel/driver/src/haru.c HARU-multi-accel/driver/include/axi_dma.h HARU-multi-accel/driver/include/dtw_accel.h HARU-multi-accel/driver/include/misc.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

$(BUILD_DIR)/axi_dma.o: HARU-multi-accel/driver/src/axi_dma.c HARU-multi-accel/driver/include/axi_dma.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

$(BUILD_DIR)/dtw_accel.o: HARU-multi-accel/driver/src/dtw_accel.c HARU-multi-accel/driver/include/dtw_accel.h HARU-multi-accel/driver/include/misc.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

slow5lib/lib/libslow5.a:
	$(MAKE) -C slow5lib zstd=$(zstd) no_simd=$(no_simd) zstd_local=$(zstd_local)  lib/libslow5.a

clean:
	rm -rf $(BINARY) $(BUILD_DIR)/*.o
	make -C slow5lib clean

# Delete all gitignored files (but not directories)
distclean: clean
	git clean -f -X
	rm -rf $(BUILD_DIR)/* autom4te.cache

test: $(BINARY)
	scripts/test.sh

