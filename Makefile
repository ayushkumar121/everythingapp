CC=clang
CFLAGS=-Wall -Wextra -g -Og
BUILD_DIR=build
SRC_DIR=src

LD_MODULE_FLAGS=-lm
LD_FLAGS_MAC=-framework Cocoa
LD_FLAGS_LINUX=-lwayland-client

MODULE_SRC_FILES=$(SRC_DIR)/everything.c $(SRC_DIR)/drawing.c $(SRC_DIR)/basic.c
MAC_SRC_FILES=$(SRC_DIR)/everything_mac.m $(SRC_DIR)/platform_mac.m
LINUX_SRC_FILES=$(SRC_DIR)/everything_wayland.c $(SRC_DIR)/platform_linux.c $(SRC_DIR)/xdg-shell-protocol.c
WIN_SRC_FILES=$(SRC_DIR)/everything_win32.c $(SRC_DIR)/platform_win32.c

$(shell mkdir -p $(BUILD_DIR))

UNAME := $(shell uname -s)

ifeq ($(UNAME), Linux)
    DEFAULT_TARGET := $(BUILD_DIR)/everything_wayland
    MODULE_NAME = everything.so
else ifeq ($(UNAME), Darwin)
    DEFAULT_TARGET := $(BUILD_DIR)/everything_mac
    MODULE_NAME = everything.dylib
else ifeq ($(UNAME), Windows_NT)
    DEFAULT_TARGET := $(BUILD_DIR)/everything_win32
    MODULE_NAME = everything.dll
else
    DEFAULT_TARGET := unknown_target
endif

.DEFAULT_GOAL := $(DEFAULT_TARGET)

all: $(BUILD_DIR)/everything_wayland $(BUILD_DIR)/everything_mac $(BUILD_DIR)/everything_win32
hotreload: $(BUILD_DIR)/$(MODULE_NAME)

$(BUILD_DIR)/everything_wayland: $(BUILD_DIR)/$(MODULE_NAME) $(LINUX_SRC_FILES)
	$(CC) -o $(BUILD_DIR)/everything_wayland $(LINUX_SRC_FILES) $(CFLAGS) $(LD_FLAGS_LINUX)

$(BUILD_DIR)/everything_mac: $(BUILD_DIR)/$(MODULE_NAME) $(MAC_SRC_FILES)
	$(CC) -o $(BUILD_DIR)/everything_mac $(SRC_DIR) $(MAC_SRC_FILES) $(CFLAGS) $(LD_FLAGS_MAC)

$(BUILD_DIR)/everything_win32: $(BUILD_DIR)/$(MODULE_NAME) $(WIN_SRC_FILES)
	@echo "ERROR: Building for win32 is not supported yet"

$(BUILD_DIR)/$(MODULE_NAME): $(SRC_DIR)/everything.c $(MODULE_SRC_FILES)
	$(CC) -shared -fPIC -o $(BUILD_DIR)/$(MODULE_NAME) $(MODULE_SRC_FILES) $(CFLAGS) $(LD_MODULE_FLAGS)

unknown_target:
	@echo "ERROR: Unknown platform"

clean:
	rm -rf build

.PHONY: clean
