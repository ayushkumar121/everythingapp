#pragma once

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  
typedef struct
{
	size_t length;
	size_t capacity;
	char *items;
} StringBuilder;

void sb_resize(StringBuilder *sb, size_t new_capacity);

void sb_free(StringBuilder *sb);

#define sb_push(sb, val)                                                       \
  _Generic((val),                                                              \
      char *: sb_push_str,                                                     \
      char: sb_push_char,                                                      \
      int: sb_push_int,                                                        \
      float: sb_push_double,                                                   \
      double: sb_push_double)(sb, val)

void sb_push_str(StringBuilder *sb, char *str);

void sb_push_char(StringBuilder *sb, char ch);

void sb_push_int(StringBuilder *sb, int i);

void sb_push_double(StringBuilder *sb, double d);

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
