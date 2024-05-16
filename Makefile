CC=clang
CFLAGS=-Wall -Wextra -g -std=c11
LD_FLAGS_MAC=-framework Cocoa
BUILD_DIR=build
SRC_DIR=src

$(shell mkdir -p $(BUILD_DIR))

all: $(BUILD_DIR)/everything_mac
hot_reload: $(BUILD_DIR)/everything.so

$(BUILD_DIR)/everything_mac: $(BUILD_DIR)/everything.so $(SRC_DIR)/everything_mac.m
	$(CC) -o $(BUILD_DIR)/everything_mac $(SRC_DIR)/everything_mac.m $(CFLAGS) $(LD_FLAGS_MAC)

$(BUILD_DIR)/everything.so: $(SRC_DIR)/everything.c
	$(CC) -shared -fPIC -o $(BUILD_DIR)/everything.so $(SRC_DIR)/everything.c $(CFLAGS)

clean:
	rm -rf build

.PHONY: clean