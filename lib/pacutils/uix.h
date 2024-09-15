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

#include <alpm_list.h>
#include <stdio.h>

#ifndef PACUTILS_UIX_H
#define PACUTILS_UIX_H

/******************************************************************************
 * Basic wrappers with error messages that exit the program on failure
 *****************************************************************************/

char *pu_uix_strdup(const char *string);

void *pu_uix_malloc(size_t size);
void *pu_uix_calloc(size_t nelem, size_t elsize);
void *pu_uix_realloc(void *ptr, size_t size);

alpm_list_t *pu_uix_list_append(alpm_list_t **list, void *data);
alpm_list_t *pu_uix_list_append_strdup(alpm_list_t **list, const char *data);

void pu_uix_read_list_from_fd_string(const char *fdstr, int sep, alpm_list_t **dest);
void pu_uix_read_list_from_path(const char *file, int sep, alpm_list_t **dest);
void pu_uix_read_list_from_stream(FILE *stream, int sep, alpm_list_t **dest, const char *label);

#endif /* PACUTILS_UIX_H */
