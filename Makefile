include classicslive-integration.mk

CC := gcc
CFLAGS_COMMON := $(CLASSICS_LIVE_CFLAGS) -I$(CLASSICS_LIVE_DIR) -I$(CLASSICS_LIVE_LIBRETRO_DIR)/include -Wall -O2 -DCL_TESTS=1 -DCL_URL_HOSTNAME=\"fake-website\"
CFLAGS_C89 := $(CFLAGS_COMMON) -std=c89 -Wall -Wextra -Wpedantic -Wno-implicit-function-declaration
LDFLAGS := -lm $(CLASSICS_LIVE_LIBS)

TARGET := cl_test

OBJS_CLASSICSLIVE := $(CLASSICS_LIVE_SOURCES_CLASSICSLIVE:.c=.o)
OBJS_LIBRETRO := $(CLASSICS_LIVE_SOURCES_LIBRETRO:.c=.o)
OBJS_TEST := $(CLASSICS_LIVE_DIR)/cl_test.o
OBJS := $(OBJS_CLASSICSLIVE) $(OBJS_LIBRETRO) $(OBJS_TEST)

.PHONY: all clean

all: $(TARGET)

$(CLASSICS_LIVE_DIR)/%.o: $(CLASSICS_LIVE_DIR)/%.c
	$(CC) $(CFLAGS_C89) -c $< -o $@

$(CLASSICS_LIVE_LIBRETRO_DIR)/%.o: $(CLASSICS_LIVE_LIBRETRO_DIR)/%.c
	$(CC) $(CFLAGS_COMMON) -c $< -o $@

$(CLASSICS_LIVE_DIR)/cl_test.o: $(CLASSICS_LIVE_DIR)/cl_test.c
	$(CC) $(CFLAGS_C89) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)
