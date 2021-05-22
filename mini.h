/*
 * Copyright 2014 Andrew Gregory <andrew.gregory.8@gmail.com>
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
 * Project URL: http://github.com/andrewgregory/mINI.c
 */

#ifndef MINI_H
#define MINI_H

#include <limits.h>
#include <stdio.h>

#if !defined(MINI_BUFFER_SIZE) || (MINI_BUFFER_SIZE < 2)
#ifdef LINE_MAX
#define MINI_BUFFER_SIZE LINE_MAX
#else
#define MINI_BUFFER_SIZE 2048
#endif
#endif

typedef struct {
    FILE *stream;
    unsigned int lineno;
    char *section, *key, *value;
    int eof;
    /* private */
    char *_buf;
    size_t _buf_size;
    int _free_stream;
} mini_t;

typedef int (*mini_cb_t)(unsigned int line, char *section,
		char *key, char *value, void *data);

mini_t *mini_finit(FILE *stream);
mini_t *mini_init(const char *path);
mini_t *mini_next(mini_t *ctx);
void mini_free(mini_t *ctx);

mini_t *mini_lookup_key(mini_t *ctx, const char *section, const char *key);

int mini_fparse_cb(FILE *stream, mini_cb_t cb, void *data);
int mini_parse_cb(const char *path, mini_cb_t cb, void *data);

#endif /* MINI_H */
