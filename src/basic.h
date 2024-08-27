#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define unused(x) ((void)(x))
#define unreachable() assert(false && "Unreachable code")
#define countof(x) (sizeof(x) / sizeof((x)[0]))

#ifdef _WIN32
#define export __declspec(dllexport)
#else
#define export __attribute__((visibility("default")))
#endif

#define ARRAY_INIT_CAP 10

#define ARRAY(T)     \
  struct             \
  {                  \
    size_t length;   \
    size_t capacity; \
    T *items;        \
  }

#define array_append(array, item)                                              \
  do {                                                                         \
    if ((array)->capacity < (array)->length + 1) {                             \
      (array)->capacity =                                                      \
          ((array)->capacity == 0) ? ARRAY_INIT_CAP : (array)->capacity * 2;   \
      (array)->items = realloc(                                            \
          (array)->items, (array)->capacity * sizeof(*(array)->items));        \
          assert((array)->items != NULL);                                      \
    }                                                                          \
    (array)->items[(array)->length++] = (item);                                \
  } while (0)

#define array_free(array)                                                      \
  do {                                                                         \
    free((array)->items);                                                      \
    (array)->items = NULL;                                                     \
    (array)->length = 0;                                                       \
    (array)->capacity = 0;                                                     \
  } while (0)

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

typedef struct
{
	size_t length;
	size_t capacity;
	char *items;
} StringBuilder;

void sb_resize(StringBuilder *sb, size_t new_capacity);

void sb_free(StringBuilder *sb);

#define sb_append(sb, val)                                                       \
  _Generic((val),                                                                \
      char *: sb_append_str,                                                     \
      char: sb_append_char,                                                      \
      int: sb_append_int,                                                        \
      float: sb_append_double,                                                   \
      double: sb_append_double)(sb, val)

void sb_append_str(StringBuilder *sb, char *str);

void sb_append_char(StringBuilder *sb, char ch);

void sb_append_int(StringBuilder *sb, int i);

void sb_append_double(StringBuilder *sb, double d);

void sb_append_null(StringBuilder *sb);

StringBuilder sb_clone(StringBuilder *sb);

typedef struct
{
	size_t length;
	char *items;
} StringView;

#define SV_Fmt "%.*s"
#define SV_Arg(sv) (int)(sv).length, (sv).items

StringView sv_from_parts(char *str, size_t len);

StringView sv_from_cstr(char *str);

StringView sv_from_sb(StringBuilder *sb);

bool sv_equal(StringView s1, StringView s2);

StringView sv_trim_left(StringView sv);

StringView sv_trim_right(StringView sv);

StringView sv_trim(StringView sv);

StringView sv_chop_delim(StringView *sv, char delim);

StringView sv_chop_str(StringView *sv, char *str);

#ifdef _WIN32
typedef HANDLE Process;
#define INVALID_PROCESS INVALID_HANDLE_VALUE 
#else
typedef int Process;
#define INVALID_PROCESS -1
#endif

bool proc_wait(Process proc);

typedef ARRAY(char*) Cmd;

void cmd_to_str(Cmd *cmd, StringBuilder *sb);
Process cmd_run_async(Cmd *cmd);
bool cmd_run_sync(Cmd *cmd);

bool file_rename(char* old_path, char* new_path);
bool needs_rebuild(char* binary_path, const char** src_files, size_t src_files_len);

#ifdef BASIC_IMPLEMENTATION

void sb_resize(StringBuilder *sb, size_t new_capacity)
{
	sb->capacity = new_capacity;
	sb->items = realloc(sb->items, sb->capacity + 1);
}

void sb_free(StringBuilder *sb)
{
	array_free(sb);
}

void sb_append_str(StringBuilder *sb, char *str)
{
	size_t item_len = strlen(str);
	if (sb->capacity < (sb->length + item_len + 1))
	{
		sb_resize(sb, sb->capacity + item_len);
	}

	memcpy(sb->items + sb->length, str, item_len);
	sb->length += item_len;
	sb->items[sb->length] = 0;
}

void sb_append_char(StringBuilder *sb, char ch)
{
	if (sb->capacity < (sb->length + 2))
	{
		sb_resize(sb, sb->capacity + 1);
	}

	sb->items[sb->length] = ch;
	sb->length += 1;
	sb->items[sb->length] = 0;
}

void sb_append_int(StringBuilder *sb, int i)
{
	if (i == 0)
	{
		sb_append_char(sb, '0');
		return;
	}

	bool is_neg = i < 0;
	if (is_neg)
		i *= -1;

	int k = (is_neg) ? log10(i) + 2 : log10(i) + 1;
	sb_resize(sb, sb->capacity + k);

	int j = k;
	while (i != 0)
	{
		sb->items[sb->length + j - 1] = i % 10 + '0';
		i = i / 10;
		j -= 1;
	}

	if (is_neg)
		sb->items[sb->length] = '-';
	sb->length += k;
	sb->items[sb->length] = 0;
}

void sb_append_double(StringBuilder *sb, double d)
{
	// Handling int part
	int i = (d < 0) ? ceil(d) : floor(d);
	sb_append_int(sb, i);

	sb_append_int(sb, '.');

	// Handling fractional part
	int f = (d - i) * 1000000;
	if (f < 0)
		f *= -1;
	sb_append_int(sb, f);
}

void sb_append_null(StringBuilder *sb)
{
	sb_append_char(sb, '\0');
}

StringBuilder sb_clone(StringBuilder *sb)
{
	StringBuilder clone = {0};
	sb_append_str(&clone, sb->items);
	return clone;
}

StringView sv_from_parts(char *str, size_t len)
{
	return (StringView)
	{
		.length = len, .items = str
	};
}

StringView sv_from_cstr(char *str)
{
	StringView sv = {0};
	sv.items = str;
	sv.length = strlen(str);
	return sv;
}

StringView sv_from_sb(StringBuilder *sb)
{
	StringView sv = {0};
	sv.items = sb->items;
	sv.length = sb->length;
	return sv;
}

bool sv_equal(StringView s1, StringView s2)
{
	if (s1.length != s2.length)
		return false;

	for (size_t i = 0; i < s1.length; i++)
	{
		if (s1.items[i] != s2.items[i])
			return false;
	}

	return true;
}

StringView sv_trim_left(StringView sv)
{
	StringView result = sv;
	while (isspace(*result.items))
	{
		result.items++;
		result.length--;
	}
	return result;
}

StringView sv_trim_right(StringView sv)
{
	StringView result = sv;
	while (isspace(result.items[result.length - 1]))
		result.length--;
	return result;
}

StringView sv_trim(StringView sv)
{
	StringView result = sv_trim_left(sv);
	result = sv_trim_right(sv);

	return result;
}

StringView sv_chop_delim(StringView *sv, char delim)
{
	size_t i = 0;
	while (sv->items[i] != delim && i < sv->length)
		i++;

	StringView result = {0};
	result.items = sv->items;
	result.length = i;

	sv->items = sv->items + i + 1;
	sv->length = sv->length - i;

	return result;
}

StringView sv_chop_str(StringView *sv, char *str)
{
	size_t n = strlen(str);
	StringView pat = sv_from_parts(str, n);
	StringView result = sv_from_parts(sv->items, 0);

	size_t i = 0;
	for (i = 0; i < sv->length - n; i++)
	{
		StringView target = sv_from_parts(&(sv->items[i]), n);
		if (sv_equal(pat, target))
			break;
	}

	result.length = i;

	sv->items = sv->items + i + n;
	sv->length = sv->length - i - n;

	return result;
}

bool proc_wait(Process proc)
{
#ifdef _WIN32
	DWORD result = WaitForSingleObject(proc, INFINITE);
	if (result == WAIT_FAILED)
	{
		fprintf(stderr, "ERROR: Failed to wait for process\n");
		return false;
	}

	DWORD exit_status;
	if (!GetExitCodeProcess(proc, &exit_status))
	{
		fprintf(stderr, "ERROR: Failed to get exit code\n");
		return false;
	}

	if (exit_status != 0)
	{
		fprintf(stderr, "ERROR: Process exited with non-zero status\n");
		return false;
	}

	CloseHandle(proc);
#else
	int wstatus;
	if (waitpid(proc, &wstatus, 0) < 0)
	{
		perror("ERROR: Failed to wait for process");
		return false;
	}
	
	if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) != 0)
	{
		fprintf(stderr, "ERROR: Process exited with non-zero status\n");
		return false;
	}
#endif
	return true;
}

void cmd_to_str(Cmd *cmd, StringBuilder *sb)
{
	for (size_t i = 0; i < cmd->length; i++)
	{
		sb_append_str(sb, cmd->items[i]);
		sb_append_char(sb, ' ');
	}
}

Process cmd_run_async(Cmd *cmd)
{
	StringBuilder sb = {0};
	cmd_to_str(cmd, &sb);
	sb_append_null(&sb);

	fprintf(stderr, "INFO: Running command: %s\n", sb.items);

#ifdef _WIN32
    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);

    siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	BOOL bSuccess = CreateProcessA(NULL, sb.items, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
	sb_free(&sb);

	if (!bSuccess)
		return INVALID_PROCESS;
	return piProcInfo.hProcess;
#else
	Process proc = fork();
	if (proc == 0)
	{
		if (execvp(cmd->items[0], cmd->items) < 0)
		{
			fprintf(stderr, "ERROR: Failed to run command: %s\n", cmd->items[0]);
			exit(1);
		}
		unreachable();
	}
	else if (proc < 0)
	{
		sb_free(&sb);
		return INVALID_PROCESS;
	}

	sb_free(&sb);
	return proc;
#endif
}

bool cmd_run_sync(Cmd *cmd)
{
	Process proc = cmd_run_async(cmd);
	if (proc == INVALID_PROCESS)
	{
		fprintf(stderr, "ERROR: Failed to run command\n");
		return false;
	}

	return proc_wait(proc);
}

bool file_rename(char* old_path, char* new_path)
{
#ifdef _WIN32
	if (MoveFileEx(old_path, new_path, MOVEFILE_REPLACE_EXISTING) == 0)
		return false;
#else
	if (rename(old_path, new_path) != 0)
		return false;
#endif
	return true;
}


int64_t get_file_mod_time(const char *file_path)
{
#ifdef _WIN32
	HANDLE file = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return -1;
		
	FILETIME last_write_time;
	BOOL bSuccess = GetFileTime(file, NULL, NULL, &last_write_time);
	CloseHandle(file);

	if (!bSuccess)
        return -1;

	ULARGE_INTEGER uli;
	uli.LowPart = last_write_time.dwLowDateTime;
	uli.HighPart = last_write_time.dwHighDateTime;

    return (int64_t)((uli.QuadPart / 10000000ULL) - 11644473600ULL);
#else
	struct stat st;
	if (stat(file_path, &st) < 0)
		return -1;

	return st.st_mtime;
#endif
}

bool needs_rebuild(char* binary_path, const char** src_files, size_t src_files_len)
{
	int out_time = get_file_mod_time(binary_path);
	if (out_time < 0)
		return true;

	for (size_t i = 0; i < src_files_len; i++)
	{
		int src_time = get_file_mod_time(src_files[i]);
		if (src_time < 0)
			return true;

		if (src_time > out_time)
			return true;
	}

	return false;
}
#endif