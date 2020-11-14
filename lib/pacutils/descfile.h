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
 */

#ifndef PACUTILS_DESCFILE_H
#define PACUTILS_DESCFILE_H

#include "pkginfo.h"

typedef enum {
  PU_DESCFILE_READER_STATUS_OK,
  PU_DESCFILE_READER_STATUS_SYSERR,
  PU_DESCFILE_READER_STATUS_UNKNOWN_FIELD,
  PU_DESCFILE_READER_STATUS_MISSING_FIELD,
  PU_DESCFILE_READER_STATUS_DUPLICATE_FIELD,
  PU_DESCFILE_READER_STATUS_INVALID_VALUE,
} pu_descfile_reader_status_t;

typedef struct {
  pu_descfile_reader_status_t status;
  FILE *stream;
  char *field, *value;
  int lineno, error, eof;

  char *_buf;        /* line buffer */
  size_t _buflen;    /* line buffer length */
  int _close_stream; /* close stream on free */
} pu_descfile_reader_t;

pu_descfile_reader_t *pu_descfile_reader_open_stream(FILE *stream);
pu_descfile_reader_t *pu_descfile_reader_open_path(const char *path);
pu_descfile_reader_t *pu_descfile_reader_open_str(char *string);

/* read the next value and store it in pkginfo if provided */
int pu_descfile_reader_next(pu_descfile_reader_t *reader, pu_pkginfo_t *pkginfo);

void pu_descfile_reader_free(pu_descfile_reader_t *reader);

#endif /* PACUTILS_DESCFILE_H */

/* vim: set ts=2 sw=2 et: */
