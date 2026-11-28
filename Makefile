CC ?= cc
CFLAGS ?= -Wall -Wextra -Werror -std=c11
CPPFLAGS ?= -I./src

SRC_DIR := src
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

SRCS := $(shell find $(SRC_DIR) -name '*.c' | sort)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

TARGET := $(BUILD_DIR)/hgen

TEST_SRCS := tests/test_scan.c tests/test_strip.c tests/test_fileio.c \
	tests/test_header.c tests/test_structs.c tests/test_source.c \
	tests/test_ignore.c tests/test_project.c
TEST_BINS := $(patsubst tests/%.c,$(BUILD_DIR)/%,$(TEST_SRCS))
FIXTURES := $(BUILD_DIR)/fixtures

SAN_CFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer

.PHONY: all clean test test-sanitize

all: $(TARGET)

test: $(TARGET) $(TEST_BINS)
	@for t in $(TEST_BINS); do ./$$t || exit 1; done
	@rm -rf $(FIXTURES)
	@mkdir -p $(FIXTURES)
	@cp tests/fixtures/kmp_search.c tests/fixtures/messy.c tests/fixtures/messy.h $(FIXTURES)/
	./$(TARGET) --verbose $(FIXTURES)/kmp_search.c | diff -u tests/expected/expected_kmp.txt -
	./$(TARGET) --verbose $(FIXTURES)/messy.c | diff -u tests/expected/expected_messy.txt -

test-sanitize:
	$(MAKE) clean
	$(MAKE) test CFLAGS="$(CFLAGS) $(SAN_CFLAGS)"

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/test_scan: tests/test_scan.c tests/test.h $(SRC_DIR)/scan/scan.c $(SRC_DIR)/scan/scan.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_scan.c

$(BUILD_DIR)/test_strip: tests/test_strip.c tests/test.h $(SRC_DIR)/core/strip.c $(SRC_DIR)/core/strip.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_strip.c $(SRC_DIR)/core/strip.c

$(BUILD_DIR)/test_fileio: tests/test_fileio.c tests/test.h $(SRC_DIR)/core/fileio.c $(SRC_DIR)/core/fileio.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_fileio.c $(SRC_DIR)/core/fileio.c

$(BUILD_DIR)/test_header: tests/test_header.c tests/test.h $(SRC_DIR)/pipeline/header.c $(SRC_DIR)/pipeline/header.h $(SRC_DIR)/pipeline/structs.c $(SRC_DIR)/pipeline/structs.h $(SRC_DIR)/scan/scan.c $(SRC_DIR)/scan/scan.h $(SRC_DIR)/core/buf.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_header.c $(SRC_DIR)/pipeline/structs.c $(SRC_DIR)/scan/scan.c $(SRC_DIR)/core/buf.c

$(BUILD_DIR)/test_structs: tests/test_structs.c tests/test.h $(SRC_DIR)/pipeline/structs.c $(SRC_DIR)/pipeline/structs.h $(SRC_DIR)/scan/scan.c $(SRC_DIR)/scan/scan.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_structs.c $(SRC_DIR)/pipeline/structs.c $(SRC_DIR)/scan/scan.c

$(BUILD_DIR)/test_source: tests/test_source.c tests/test.h $(SRC_DIR)/pipeline/source.c $(SRC_DIR)/pipeline/source.h $(SRC_DIR)/pipeline/structs.c $(SRC_DIR)/pipeline/structs.h $(SRC_DIR)/scan/scan.c $(SRC_DIR)/scan/scan.h $(SRC_DIR)/core/buf.c $(SRC_DIR)/core/fileio.c $(SRC_DIR)/core/fileio.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_source.c $(SRC_DIR)/pipeline/source.c $(SRC_DIR)/pipeline/structs.c $(SRC_DIR)/scan/scan.c $(SRC_DIR)/core/buf.c $(SRC_DIR)/core/fileio.c

$(BUILD_DIR)/test_ignore: tests/test_ignore.c tests/test.h $(SRC_DIR)/project/ignore.c $(SRC_DIR)/project/ignore.h $(SRC_DIR)/project/project.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_ignore.c $(SRC_DIR)/project/ignore.c

$(BUILD_DIR)/test_project: tests/test_project.c tests/test.h $(SRC_DIR)/project/project.c $(SRC_DIR)/project/project.h $(SRC_DIR)/project/ignore.c $(SRC_DIR)/project/ignore.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_project.c $(SRC_DIR)/project/project.c $(SRC_DIR)/project/ignore.c

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
