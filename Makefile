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

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
