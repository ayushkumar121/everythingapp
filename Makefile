CC=clang

LD_FLAGS_MAC=-framework Cocoa

everything_mac: everything_mac.m
	$(CC) -o everything_mac everything_mac.m $(LD_FLAGS_MAC)
