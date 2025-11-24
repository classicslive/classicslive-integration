include classicslive-integration.mk

CC := gcc
CFLAGS := -I$(CLASSICS_LIVE_DIR) -I$(CLASSICS_LIVE_LIBRETRO_DIR)/include -Wall -O2 -DCL_TESTS=1
LDFLAGS := -lm

TARGET := cl_test
SOURCES := $(CLASSICS_LIVE_SOURCES) $(CLASSICS_LIVE_DIR)/cl_test.c
OBJS := $(SOURCES:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)
