/*
 * Copyright 2020 Andrew Gregory <andrew.gregory.8@gmail.com>
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
 *
 * Project URL: http://github.com/andrewgregory/mGarbageCollector.c
 */

#ifndef MGARBAGECOLLECTOR_H
#define MGARBAGECOLLECTOR_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

typedef void (mgc_fn_free_t) (void*);
typedef void (mgc_fn_fail_t) (void*);

typedef struct mgc_item_t {
    mgc_fn_free_t *free;
    void *data;
    struct mgc_item_t *next;
} mgc_item_t;

typedef struct mgc_t {
    mgc_item_t *items;
    mgc_item_t *last;
    mgc_fn_fail_t *failfn;
    void *failctx;
} mgc_t;

mgc_t *mgc_new(mgc_fn_fail_t *failfn, void *failctx);
void *mgc_add(mgc_t *mgc, void *ptr, mgc_fn_free_t* fn);
mgc_fn_free_t *mgc_remove_item(mgc_t *mgc, void *ptr);
void mgc_free_item(mgc_t *mgc, void *ptr);
void mgc_clear(mgc_t *mgc);
void mgc_free(mgc_t *mgc);

void *mgc_malloc(mgc_t *mgc, size_t size);
void *mgc_calloc(mgc_t *mgc, size_t nelem, size_t elsize);
void *mgc_realloc_ptr(mgc_t *mgc, void *ptr, size_t size);

char *mgc_strdup(mgc_t *mgc, const char *s);
char *mgc_strndup(mgc_t *mgc, const char *s, size_t size);
char *mgc_vasprintf(mgc_t *mgc, const char *restrict fmt, va_list ap);
char *mgc_asprintf(mgc_t *mgc, const char *restrict fmt, ...);

void mgc_close(void *fdp);
int mgc_openat(mgc_t *mgc, int fd, const char *path, int oflag, ...);
int mgc_open(mgc_t *mgc, const char *path, int oflag, ...);

void mgc_fclose(void *stream);
FILE *mgc_fopen(mgc_t *mgc, const char *restrict pathname, const char *restrict mode);
FILE *mgc_fdopen(mgc_t *mgc, int fildes, const char *mode);
FILE *mgc_fmemopen(mgc_t *mgc, void *restrict buf, size_t size, const char *mode);
FILE *mgc_open_memstream(mgc_t *mgc, char **ptr, size_t *sizeloc);

#endif /* MGARBAGECOLLECTOR_H */
