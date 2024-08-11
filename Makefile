BUILD_DIR = build
CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Wno-unused-function -Iinclude
CFLAGS += -Wno-unused-parameter -Wno-sign-compare
LDFLAGS =
FUSE_CFLAGS = $(shell pkg-config fuse3 --cflags)
FUSE_LDFLAGS = $(shell pkg-config fuse3 --libs)
SOURCES = src/aaFat.c src/example.c src/printfat.c src/fuse_example.c src/example2.c

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(SOURCES:.c=.o)))
DEP = $(OBJECTS:%.o=%.d)
vpath %.c $(sort $(dir $(SOURCES)))
vpath %.o $(BUILD_DIR)

.PHONY: all clean example example2 printfat fuse_example fuse_example_test

all: printfat fuse_example example

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJECTS): $(BUILD_DIR)

$(BUILD_DIR)/fuse_example.o: CFLAGS+=$(FUSE_CFLAGS)

-include $(DEP)
$(BUILD_DIR)/%.o: %.c
	$(CC) -MMD -c $(CFLAGS) $< -o $@

example: aaFat.o example.o
	$(CC) $(LDFLAGS) $^ -o $@

example2: CFLAGS += -include example2.h
example2: aaFat.o example2.o
	$(CC) $(LDFLAGS) $^ -o $@

printfat: aaFat.o  printfat.o
	$(CC) $(LDFLAGS) $^ -o $@

fuse_example: aaFat.o fuse_example.o
	$(CC) $^ $(FUSE_CFLAGS) $(FUSE_LDFLAGS) $(LDFLAGS) -o $@

fuse_example_test:
	mkdir -p test
	./fuse_example -f test

clean:
	rm -rf $(BUILD_DIR)/* example printfat fuse_example
