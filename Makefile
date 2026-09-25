# PLATFORM is detected from the host and can be overridden on the command line:
#
#   make PLATFORM=PLATFORM_LINUX
#
# It is one of PLATFORM_WIN32, PLATFORM_DARWIN or PLATFORM_LINUX, and is also passed
# to the compiler as a define. Run `make clean` after switching platforms.

LIB_SOURCES = src/everything.c src/drawing.c src/views.c
HEADERS = $(wildcard src/*.h)

ifndef PLATFORM
    ifeq ($(OS),Windows_NT)
        PLATFORM = PLATFORM_WIN32
    else ifeq ($(shell uname -s),Darwin)
        PLATFORM = PLATFORM_DARWIN
    else
        PLATFORM = PLATFORM_LINUX
    endif
endif

ifeq ($(PLATFORM),PLATFORM_WIN32)
    # cl.exe from a Visual Studio developer prompt, with GNU make and a POSIX shell (Git Bash, MSYS2)
    LIB = everything.dll
    EXE = everything.exe
    EXE_SOURCES = src/everything_win32.c src/hotreload.c
else ifeq ($(PLATFORM),PLATFORM_DARWIN)
    LIB = everything.dylib
    EXE = everything
    LIB_FLAGS = -dynamiclib
    EXE_SOURCES = src/everything_mac.m src/hotreload.c
    EXE_LIBS = -framework Cocoa -framework QuartzCore
else ifeq ($(PLATFORM),PLATFORM_LINUX)
    LIB = everything.so
    EXE = everything
    LIB_FLAGS = -shared -fPIC
    EXE_SOURCES = src/everything_wayland.c src/xdg-shell-protocol.c src/hotreload.c
    EXE_LIBS = -lwayland-client -ldl
else
    $(error Unknown PLATFORM '$(PLATFORM)', use PLATFORM_WIN32, PLATFORM_DARWIN or PLATFORM_LINUX)
endif

.PHONY: all lib run clean

all: $(LIB) $(EXE)

lib: $(LIB)

run: all
	./$(EXE)

ifeq ($(PLATFORM),PLATFORM_WIN32)

# A debugger locks the loaded DLL's PDB, so every build gets a fresh one
$(LIB): $(LIB_SOURCES) $(HEADERS)
	cl.exe /nologo /D$(PLATFORM) /LD /Zi /O2 $(LIB_SOURCES) /Fe:$@ /link /PDB:everything_$(shell date +%s).pdb

$(EXE): $(EXE_SOURCES) $(HEADERS)
	cl.exe /nologo /D$(PLATFORM) /Zi /O2 $(EXE_SOURCES) User32.lib Gdi32.lib /Fe:$@

else

$(LIB): $(LIB_SOURCES) $(HEADERS)
	$(CC) -Wall -Wextra -Wpedantic -g -O2 -D$(PLATFORM) $(LIB_FLAGS) -o $@ $(LIB_SOURCES)

$(EXE): $(EXE_SOURCES) $(HEADERS)
	$(CC) -Wall -Wextra -g -O2 -D$(PLATFORM) -o $@ $(EXE_SOURCES) $(EXE_LIBS)

endif

clean:
	rm -rf everything everything.dylib everything.so *.dSYM
	rm -f everything.exe everything.dll everything_loaded.dll everything*.pdb *.ilk *.exp *.lib *.obj vc*.pdb
