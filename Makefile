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
	$(SRC_DIR)/structs.c \
	$(SRC_DIR)/source.c \
	$(SRC_DIR)/header.c
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

TARGET := $(BUILD_DIR)/hgen

TEST_SRCS := tests/test_scan.c tests/test_strip.c tests/test_fileio.c \
	tests/test_header.c tests/test_structs.c tests/test_source.c
TEST_BINS := $(patsubst tests/%.c,$(BUILD_DIR)/%,$(TEST_SRCS))
FIXTURES := $(BUILD_DIR)/fixtures

SAN_CFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer

.PHONY: all clean test test-sanitize

all: $(TARGET)

test: $(TARGET) $(TEST_BINS)
	@for t in $(TEST_BINS); do ./$$t || exit 1; done
	@rm -rf $(FIXTURES)
	@mkdir -p $(FIXTURES)
	@cp tests/kmp_search.c tests/messy.c $(FIXTURES)/
	./$(TARGET) $(FIXTURES)/kmp_search.c | diff -u tests/expected_kmp.txt -
	./$(TARGET) $(FIXTURES)/messy.c | diff -u tests/expected_messy.txt -

test-sanitize:
	$(MAKE) clean
	$(MAKE) test CFLAGS="$(CFLAGS) $(SAN_CFLAGS)"

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

# test_scan and test_header include an implementation file to reach its
# file-local helpers; do not link those files twice.
$(BUILD_DIR)/test_scan: tests/test_scan.c tests/test.h $(SRC_DIR)/scan.c $(SRC_DIR)/scan.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_scan.c

$(BUILD_DIR)/test_strip: tests/test_strip.c tests/test.h $(SRC_DIR)/strip.c $(SRC_DIR)/strip.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_strip.c $(SRC_DIR)/strip.c

$(BUILD_DIR)/test_fileio: tests/test_fileio.c tests/test.h $(SRC_DIR)/fileio.c $(SRC_DIR)/fileio.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_fileio.c $(SRC_DIR)/fileio.c

$(BUILD_DIR)/test_header: tests/test_header.c tests/test.h $(SRC_DIR)/header.c $(SRC_DIR)/header.h $(SRC_DIR)/structs.c $(SRC_DIR)/structs.h $(SRC_DIR)/scan.c $(SRC_DIR)/scan.h $(SRC_DIR)/buf.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_header.c $(SRC_DIR)/structs.c $(SRC_DIR)/scan.c $(SRC_DIR)/buf.c

$(BUILD_DIR)/test_structs: tests/test_structs.c tests/test.h $(SRC_DIR)/structs.c $(SRC_DIR)/structs.h $(SRC_DIR)/scan.c $(SRC_DIR)/scan.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_structs.c $(SRC_DIR)/structs.c $(SRC_DIR)/scan.c

$(BUILD_DIR)/test_source: tests/test_source.c tests/test.h $(SRC_DIR)/source.c $(SRC_DIR)/source.h $(SRC_DIR)/structs.c $(SRC_DIR)/structs.h $(SRC_DIR)/scan.c $(SRC_DIR)/scan.h $(SRC_DIR)/buf.c $(SRC_DIR)/fileio.c $(SRC_DIR)/fileio.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_source.c $(SRC_DIR)/source.c $(SRC_DIR)/structs.c $(SRC_DIR)/scan.c $(SRC_DIR)/buf.c $(SRC_DIR)/fileio.c

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
