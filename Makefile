CC ?= cc
CFLAGS ?= -Wall -Wextra -Werror -std=c11
CPPFLAGS ?= -I./src

SRC_DIR := src
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

SRCS := \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/fileio.c \
	$(SRC_DIR)/strip.c \
	$(SRC_DIR)/scan.c \
	$(SRC_DIR)/buf.c \
	$(SRC_DIR)/header.c
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

TARGET := $(BUILD_DIR)/hgen

TEST_SRCS := tests/test_scan.c tests/test_strip.c tests/test_fileio.c tests/test_header.c
TEST_BINS := $(patsubst tests/%.c,$(BUILD_DIR)/%,$(TEST_SRCS))

.PHONY: all clean test

all: $(TARGET)

test: $(TARGET) $(TEST_BINS)
	@for t in $(TEST_BINS); do ./$$t || exit 1; done
	./$(TARGET) tests/kmp_search.c | diff -u tests/expected_kmp.txt -
	./$(TARGET) tests/messy.c | diff -u tests/expected_messy.txt -

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

# test_scan includes scan.c so it can call the file-local helpers directly;
# do not link scan.o here.
$(BUILD_DIR)/test_scan: tests/test_scan.c tests/test.h $(SRC_DIR)/scan.c $(SRC_DIR)/scan.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_scan.c

$(BUILD_DIR)/test_strip: tests/test_strip.c tests/test.h $(SRC_DIR)/strip.c $(SRC_DIR)/strip.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_strip.c $(SRC_DIR)/strip.c

$(BUILD_DIR)/test_fileio: tests/test_fileio.c tests/test.h $(SRC_DIR)/fileio.c $(SRC_DIR)/fileio.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_fileio.c $(SRC_DIR)/fileio.c

# test_header includes header.c to reach the file-local guard_name helper.
$(BUILD_DIR)/test_header: tests/test_header.c tests/test.h $(SRC_DIR)/header.c $(SRC_DIR)/header.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_header.c

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
