/*
 * Copyright 2012-2015 Andrew Gregory <andrew.gregory.8@gmail.com>
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

#include <alpm.h>

#ifndef PACUTILS_MTREE_H
#define PACUTILS_MTREE_H

typedef struct pu_mtree_t {
  char *path;
  char type[16];
  uid_t uid;
  gid_t gid;
  mode_t mode;
  off_t size;
  char md5digest[33];
  char sha256digest[65];
} pu_mtree_t;

typedef struct {
  FILE *stream;
  int eof;
  pu_mtree_t defaults;

  char *_buf;        /* line buffer */
  size_t _buflen;    /* line buffer length */
  char *_stream_buf; /* buffer for in-memory streams */
  int _close_stream; /* close stream on free */
} pu_mtree_reader_t;

__attribute__((__deprecated__))
alpm_list_t *pu_mtree_load_pkg_mtree(alpm_handle_t *handle, alpm_pkg_t *pkg);

pu_mtree_reader_t *pu_mtree_reader_open_stream(FILE *stream);
pu_mtree_reader_t *pu_mtree_reader_open_file(const char *path);
pu_mtree_reader_t *pu_mtree_reader_open_package(alpm_handle_t *h,
    alpm_pkg_t *p);
pu_mtree_t *pu_mtree_reader_next(pu_mtree_reader_t *reader, pu_mtree_t *dest);
pu_mtree_t *pu_mtree_new(void);
void pu_mtree_reader_free(pu_mtree_reader_t *reader);
void pu_mtree_free(pu_mtree_t *mtree);

#endif /* PACUTILS_MTREE_H */

/* vim: set ts=2 sw=2 et: */
