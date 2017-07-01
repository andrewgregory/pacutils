/*
 * Copyright 2012-2016 Andrew Gregory <andrew.gregory.8@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#define _XOPEN_SOURCE 700 /* strptime */

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "util.h"

char *pu_basename(char *path)
{
  char *c;
  if(!path) { return NULL; }
  for(c = path + strlen(path); c > path && *(c - 1) != '/'; --c);
  return c;
}

char *pu_hr_size(off_t bytes, char *dest)
{
  static char *suff[] = {"B", "K", "M", "G", "T", "P", "E", NULL};
  float hrsize;
  int s = 0;
  while((bytes >= 1000000 || bytes <= -1000000) && suff[s + 1]) {
    bytes /= 1024;
    ++s;
  }
  hrsize = bytes;
  if((hrsize >= 1000 || hrsize <= -1000) && suff[s + 1]) {
    hrsize /= 1024;
    ++s;
  }
  sprintf(dest, "%.2f %s", hrsize, suff[s]);
  return dest;
}

void *_pu_list_shift(alpm_list_t **list)
{
  alpm_list_t *l = *list;
  void *data;
  if(l == NULL) { return NULL; }
  data = l->data;
  if(l->next) { l->next->prev = l->prev; }
  *list = l->next;
  free(l);
  return data;
}

struct tm *pu_parse_datetime(const char *string, struct tm *stm)
{
  const char *c, *end;
  memset(stm, 0, sizeof(struct tm));
  stm->tm_isdst = -1;
  stm->tm_mday = 1;

  /* locate the end of the usable date */
  if((c = strpbrk(string, " T")) && (c = strpbrk(c, ",.Z-+"))) {
    /* ignore trailing timezone and/or fractional elements */
    end = c;
  } else {
    end = string + strlen(string);
  }

  c = string;
#define pu_parse_bit(s, f, t) \
  do { \
    if(!(s = strptime(s, f, t))) { \
      return NULL; \
    } \
    if(s == end) { \
      return stm; \
    } else if(s > end) { \
      /* no idea how we got here, but better safe than sorry */ \
      return NULL; \
    } \
  } while(0)
  pu_parse_bit(c, "%Y", stm);
  pu_parse_bit(c, "-%m", stm);
  pu_parse_bit(c, "-%d", stm);
  if(c[0] == ' ' || c[0] == 'T') { c++; }
  pu_parse_bit(c, "%H", stm);
  pu_parse_bit(c, ":%M", stm);
  pu_parse_bit(c, ":%S", stm);
#undef pu_parse_bit

  return NULL;
}

alpm_list_t *pu_list_append_str(alpm_list_t **list, const char *str)
{
  alpm_list_t *ret;
  char *dup;
  if((dup = strdup(str)) && (ret = alpm_list_append(list, dup))) {
    return ret;
  } else {
    free(dup);
    return NULL;
  }
}

char *pu_vasprintf(const char *fmt, va_list args)
{
  va_list args_copy;
  char *p;
  int len;

  va_copy(args_copy, args);
  len = vsnprintf(NULL, 0, fmt, args_copy);
  va_end(args_copy);

#if SIZE_MAX <= INT_MAX
  if(len >= SIZE_MAX) {
    errno = EOVERFLOW;
    return NULL;
  }
#endif

  if(len < 0) {
    return NULL;
  }

  if((p = malloc((size_t)len + 1)) == NULL) {
    return NULL;
  }

  vsprintf(p, fmt, args);

  return p;
}

char *pu_asprintf(const char *fmt, ...)
{
  char *p;
  va_list args;

  va_start(args, fmt);
  p = pu_vasprintf(fmt, args);
  va_end(args);

  return p;
}

char *pu_prepend_dir(const char *dir, const char *path)
{
  const char *sep = dir[strlen(dir) - 1] == '/' ? "" : "/";
  while(path[0] == '/') { path++; }
  return pu_asprintf("%s%s%s", dir, sep, path);
}

int pu_prepend_dir_list(const char *dir, alpm_list_t *paths)
{
  while(paths) {
    char *newval = pu_prepend_dir(dir, paths->data);
    if(newval == NULL) { return -1; }
    free(paths->data);
    paths->data = newval;
    paths = paths->next;
  }
  return 0;
}

FILE *pu_fopenat(int dirfd, const char *path, const char *mode)
{
  int fd, flags = 0, rwflag = 0;
  FILE *stream;
  const char *m = mode;
  switch(*(m++)) {
    case 'r': rwflag = O_RDONLY; break;
    case 'w': rwflag = O_WRONLY; flags |= O_CREAT | O_TRUNC; break;
    case 'a': rwflag = O_WRONLY; flags |= O_CREAT | O_APPEND; break;
    default: errno = EINVAL; return NULL;
  }
  if(m[1] == 'b') { m++; }
  if(m[1] == '+') { m++; rwflag = O_RDWR; }
  while(*m) {
    switch(*(m++)) {
      case 'e': flags |= O_CLOEXEC; break;
      case 'x': flags |= O_EXCL; break;
    }
  }
  if((fd = openat(dirfd, path, flags | rwflag, 0666)) < 0) { return NULL; }
  if((stream = fdopen(fd, mode)) == NULL) { close(fd); return NULL; }
  return stream;
}

/* vim: set ts=2 sw=2 et: */
