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

#ifndef PACUTILS_UTIL_H
#define PACUTILS_UTIL_H

#include <stdio.h>
#include <time.h>

#include <alpm_list.h>

char *pu_basename(char *path);
char *pu_hr_size(off_t bytes, char *dest);
struct tm *pu_parse_datetime(const char *string, struct tm *stm);

void *_pu_list_shift(alpm_list_t **list);
alpm_list_t *pu_list_append_str(alpm_list_t **list, const char *str);

char *pu_vasprintf(const char *fmt, va_list args);
char *pu_asprintf(const char *fmt, ...);

char *pu_prepend_dir(const char *dir, const char *path);
int pu_prepend_dir_list(const char *dir, alpm_list_t *paths);

FILE *pu_fopenat(int dirfd, const char *path, const char *mode);

#endif /* PACUTILS_UTIL_H */

/* vim: set ts=2 sw=2 et: */
