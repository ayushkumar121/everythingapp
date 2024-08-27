#define BASIC_IMPLEMENTATION
#include "basic.h"

void compile_self(int argc, char **argv);
void compile_library(void);
void compile_executable(void);

int main(int argc, char **argv)
{
    compile_self(argc, argv);
    compile_library();
    compile_executable();

    fprintf(stderr, "INFO: Compilation successful\n");

    return 0;
}

void compile_self(int argc, char **argv)
{
    unused(argc);

    char* binary_path = argv[0];
    char* source_path = __FILE__;

    if (needs_rebuild(binary_path, &source_path, 1))
    {
        StringBuilder sb = {0};
        sb_append_str(&sb, binary_path);
        sb_append_str(&sb, ".old");
        sb_append_null(&sb);

        if (!file_rename(binary_path, sb.items))
        {
            fprintf(stderr, "ERROR: Failed to rename old binary %lu\n", GetLastError());
            exit(1);
        }
        sb_free(&sb);

        Cmd cmd = {0};

        #ifdef _WIN32
            array_append(&cmd, "cl.exe");
            array_append(&cmd, "/nologo");
            array_append(&cmd, source_path);
            array_append(&cmd, "/Fe:");
            array_append(&cmd, binary_path);
        #else
            array_append(&cmd, "cc");
            array_append(&cmd, "-Wall -Wextra -g -Og");
            array_append(&cmd, "-o");
            array_append(&cmd, binary_path);
            array_append(&cmd, source_path);
        #endif

        bool success = cmd_run_sync(&cmd);
        array_free(&cmd);
    
        if (!success)
        {
           fprintf(stderr, "ERROR: Failed to compile self\n");
           exit(1);
        }

        Cmd cmd2 = {0};
        array_append(&cmd2, binary_path);

        success = cmd_run_sync(&cmd2);
        array_free(&cmd2);

        if (!success)
        {
            fprintf(stderr, "ERROR: Failed to run new version\n");
            exit(1);
        }
        exit(0);
    }
}

void compile_library()
{
    char* src_files[] = {
        "src/everything.c",
        "src/drawing.c",
        "src/views.c",
    };
    int src_files_count = countof(src_files);

#ifdef _WIN32
    char* lib_name = "everything.dll";
#endif
#ifdef __APPLE__
    char* lib_name = "everything.dylib";
#endif
#ifdef __linux__
    char* lib_name = "everything.so";
#endif

    if (needs_rebuild(lib_name, src_files, src_files_count))
    {
        Cmd cmd = {0};
    #ifdef _WIN32
        array_append(&cmd, "cl.exe");
        array_append(&cmd, "/nologo");
        array_append(&cmd, "/LD");
        array_append(&cmd, "/Zi");
        array_append(&cmd, "src/everything.c");
        array_append(&cmd, "src/drawing.c");
        array_append(&cmd, "src/views.c");
    #else
        array_append(&cmd, "cc");
        array_append(&cmd, "-Wall -Wextra -g -Og");
        array_append(&cmd, "-shared -fPIC");
        array_append(&cmd, "-o");
        array_append(&cmd, lib_name);        
        array_append(&cmd, "src/everything.c");
        array_append(&cmd, "src/drawing.c");
        array_append(&cmd, "src/views.c");
    #endif

        bool success = cmd_run_sync(&cmd);
        array_free(&cmd);

        if (!success)
        {
            fprintf(stderr, "ERROR: Failed to compile executable\n");
            exit(1);
        }
    }
}

void compile_executable()
{
#ifdef _WIN32
    char* src_files[] = {
        "src/everything_win32.c",
        "src/hotreload.c",
    };
    int src_files_count = countof(src_files);
    char* exe_name = "everything_win32.exe";
#endif
#ifdef __APPLE__
    char* src_files[] = {
        "src/everything_mac.c",
        "src/hotreload.c",
    };
    int src_files_count = countof(src_files);
    char* exe_name = "everything_mac";
#endif
#ifdef __linux__
    char* src_files[] = {
        "src/everything_wayland.c",
        "src/hotreload.c",
    };
    int src_files_count = countof(src_files);
    char* exe_name = "everything_wayland";
#endif

    if (needs_rebuild(exe_name, src_files, src_files_count))
    {
        Cmd cmd = {0};
    #ifdef _WIN32
        array_append(&cmd, "cl.exe");
        array_append(&cmd, "/nologo");
        array_append(&cmd, "/Zi");
        array_append(&cmd, "/Fe:");
        array_append(&cmd, exe_name);
        for (int i = 0; i < src_files_count; i++)
        {
            array_append(&cmd, src_files[i]);
        }
        array_append(&cmd, "User32.lib");
        array_append(&cmd, "Gdi32.lib");
    #else
        array_append(&cmd, "cc");
        array_append(&cmd, "-Wall -Wextra -g -Og");
        array_append(&cmd, "-o");
        array_append(&cmd, "everything");
        for (int i = 0; i < src_files_count; i++)
        {
            array_append(&cmd, src_files[i]);
        }
    #ifdef __APPLE__
        array_append(&cmd, "-framework");
        array_append(&cmd, "Cocoa");
    #else
        array_append(&cmd, "-lwayland-client");
    #endif
    #endif

        bool success = cmd_run_sync(&cmd);
        array_free(&cmd);

        if (!success)
        {
            fprintf(stderr, "ERROR: Failed to compile executable\n");
            exit(1);
        }
    }
}