/*
 * Copyright 2024 Andrew Gregory <andrew.gregory.8@gmail.com>
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

#include <errno.h>
#include <string.h>

#include "ui.h"
#include "uix.h"

static void pu_uix_assert(int success) {
  if(!success) {
    pu_ui_error("fatal error (%s)", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

static void *pu_uix_assertp(void *success) {
  pu_uix_assert(success != NULL);
  return success;
}

char *pu_uix_strdup(const char *string) {
  return pu_uix_assertp(strdup(string));
}

void *pu_uix_malloc(size_t size) {
  return pu_uix_assertp(malloc(size));
}
void *pu_uix_calloc(size_t nelem, size_t elsize) {
  return pu_uix_assertp(calloc(nelem, elsize));
}
void *pu_uix_realloc(void *ptr, size_t size) {
  return pu_uix_assertp(realloc(ptr, size));
}

alpm_list_t *pu_uix_list_append(alpm_list_t **list, void *data) {
  return pu_uix_assertp(alpm_list_append(list, data));
}

alpm_list_t *pu_uix_list_append_strdup(alpm_list_t **list, const char *data) {
  return pu_uix_assertp(alpm_list_append_strdup(list, data));
}

void pu_uix_read_list_from_fdstr(const char *fdstr, int sep, alpm_list_t **dest) {
  pu_uix_assert(pu_ui_read_list_from_fdstr(fdstr, sep, dest) == 0);
}
void pu_uix_read_list_from_path(const char *file, int sep, alpm_list_t **dest) {
  pu_uix_assert(pu_ui_read_list_from_path(file, sep, dest) == 0);
}
void pu_uix_read_list_from_stream(FILE *stream, int sep, alpm_list_t **dest, const char *label) {
  pu_uix_assert(pu_ui_read_list_from_stream(stream, sep, dest, label) == 0);
}
