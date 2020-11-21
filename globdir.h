/*
 * Copyright 2017-2020 Andrew Gregory <andrew.gregory.8@gmail.com>
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

#ifndef GLOBDIR_H
#define GLOBDIR_H

#include <glob.h>

typedef glob_t globdir_t;

void globdirfree(globdir_t *g);

int globat(int fd, const char *pattern, int flags,
        int (*errfunc) (const char *epath, int eerrno), globdir_t *pglob);

int globdir(const char *dir, const char *pattern, int flags,
        int (*errfunc) (const char *epath, int eerrno), globdir_t *pglob);

int globdir_str_is_pattern(const char *string, int noescape);

char *globdir_escape_pattern(const char *pattern);

#endif /* GLOBDIR_H */
