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

    if (file_needs_rebuild(binary_path, &source_path, 1))
    {
        StringBuilder sb = {0};
        sb_append_str(&sb, binary_path);
        sb_append_str(&sb, ".old");
        sb_append_null(&sb);

        if (!file_rename(binary_path, sb.items))
        {
            fprintf(stderr, "ERROR: Failed to rename old binary\n");
            exit(1);
        }
        sb_free(&sb);

        Cmd cmd = {0};

        #ifdef _WIN32
            array_append(&cmd, "cl.exe");
            array_append(&cmd, "/nologo");
            array_append(&cmd, "/Zi");
            array_append(&cmd, source_path);
            array_append(&cmd, "/Fe:");
            array_append(&cmd, binary_path);
        #else
            array_append(&cmd, "cc");
            array_append(&cmd, "-Wall");
            array_append(&cmd, "-Wextra");
            array_append(&cmd, "-Wpedantic");
            array_append(&cmd, "-O2");
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

        cmd_run_sync(&cmd2);
        exit(0);
    }
}

void compile_library(void)
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

    if (file_needs_rebuild(lib_name, src_files, src_files_count))
    {
        Cmd cmd = {0};
    #ifdef _WIN32
        array_append(&cmd, "cl.exe");
        array_append(&cmd, "/nologo");
        array_append(&cmd, "/LD");
        array_append(&cmd, "/DLL");
        array_append(&cmd, "/Zi");
        array_append(&cmd, "/O2");
        for (size_t i = 0; i < src_files_count; i++)
        {
            array_append(&cmd, src_files[i]);
        }
        array_append(&cmd, "/Fe:");
        array_append(&cmd, lib_name);
    #else
        array_append(&cmd, "cc");
        array_append(&cmd, "-Wall");
        array_append(&cmd, "-Wextra");
        array_append(&cmd, "-Wpedantic");
        array_append(&cmd, "-g");
        array_append(&cmd, "-O2");
    #ifdef __APPLE__
        array_append(&cmd, "-dynamiclib");
    #else
        array_append(&cmd, "-shared");
        array_append(&cmd, "-fPIC");
    #endif
        array_append(&cmd, "-o");
        array_append(&cmd, lib_name);
        for (int i = 0; i < src_files_count; i++)
        {
            array_append(&cmd, src_files[i]);
        }
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

void compile_executable(void)
{
#ifdef _WIN32
    char* src_files[] = {
        "src/everything_win32.c",
        "src/hotreload.c",
    };
    int src_files_count = countof(src_files);
    char* exe_name = "everything.exe";
#endif
#ifdef __APPLE__
    char* src_files[] = {
        "src/everything_mac.m",
        "src/hotreload.c",
    };
    int src_files_count = countof(src_files);
    char* exe_name = "everything";
#endif
#ifdef __linux__
    char* src_files[] = {
        "src/everything_wayland.c",
        "src/hotreload.c",
    };
    int src_files_count = countof(src_files);
    char* exe_name = "everything";
#endif

    if (file_needs_rebuild(exe_name, src_files, src_files_count))
    {
        Cmd cmd = {0};
    #ifdef _WIN32
        array_append(&cmd, "cl.exe");
        array_append(&cmd, "/nologo");
        array_append(&cmd, "/Zi");
        array_append(&cmd, "/O2");
        for (int i = 0; i < src_files_count; i++)
        {
            array_append(&cmd, src_files[i]);
        }
        array_append(&cmd, "User32.lib");
        array_append(&cmd, "Gdi32.lib");
        array_append(&cmd, "/Fe:");
        array_append(&cmd, exe_name);
    #else
        array_append(&cmd, "cc");
        array_append(&cmd, "-Wall");
        array_append(&cmd, "-Wextra");
        array_append(&cmd, "-g");
        array_append(&cmd, "-O2");
        array_append(&cmd, "-o");
        array_append(&cmd, exe_name);
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
