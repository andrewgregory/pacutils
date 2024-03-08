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

#include <limits.h>
#include <string.h>

#include <archive.h>

#include "mtree.h"
#include "util.h"

alpm_list_t *pu_mtree_load_pkg_mtree(alpm_handle_t *handle, alpm_pkg_t *pkg) {
  alpm_list_t *entries = NULL;
  pu_mtree_reader_t *reader;
  pu_mtree_t *e;

  if ((reader = pu_mtree_reader_open_package(handle, pkg)) == NULL) {
    return NULL;
  }

  while ((e = pu_mtree_reader_next(reader, NULL))) {
    if (alpm_list_append(&entries, e) == NULL) { goto error; }
  }
  if (!reader->eof) { goto error; }

  pu_mtree_reader_free(reader);
  return entries;

error:
  alpm_list_free_inner(entries, (alpm_list_fn_free)pu_mtree_free);
  alpm_list_free(entries);
  pu_mtree_reader_free(reader);
  return NULL;
}

pu_mtree_t *pu_mtree_new(void) {
  return calloc(sizeof(pu_mtree_t), 1);
}

void pu_mtree_free(pu_mtree_t *mtree) {
  if (mtree) {
    free(mtree->path);
    free(mtree);
  }
}

void pu_mtree_reader_free(pu_mtree_reader_t *reader) {
  if (reader == NULL) { return; }
  if (reader->_close_stream) { fclose(reader->stream); }
  free(reader->_buf);
  free(reader->_stream_buf);
  free(reader);
}

pu_mtree_reader_t *pu_mtree_reader_open_stream(FILE *stream) {
  pu_mtree_reader_t *r = calloc(sizeof(pu_mtree_reader_t), 1);
  if (r) { r->stream = stream; }
  return r;
}

pu_mtree_reader_t *pu_mtree_reader_open_file(const char *path) {
  pu_mtree_reader_t *r = calloc(sizeof(pu_mtree_reader_t), 1);
  if (r == NULL) { return NULL; }
  if ((r->stream = fopen(path, "r")) == NULL) { free(r); return NULL; }
  r->_close_stream = 1;
  return r;
}

pu_mtree_reader_t *pu_mtree_reader_open_package( alpm_handle_t *h,
    alpm_pkg_t *p) {
  pu_mtree_reader_t *reader;
  struct archive *mtree;
  char path[PATH_MAX];
  struct archive_entry *entry = NULL;
  char *buf, rbuf[256];
  size_t len;
  FILE *fbuf;
  const char *dbpath = alpm_option_get_dbpath(h);
  const char *pkgname = alpm_pkg_get_name(p);
  const char *pkgver = alpm_pkg_get_version(p);

  if ((fbuf = open_memstream(&buf, &len)) == NULL) { return NULL; }

  sprintf(path, "%slocal/%s-%s/mtree", dbpath, pkgname, pkgver);

  if ((mtree = archive_read_new()) == NULL) { return NULL; }
  archive_read_support_filter_all(mtree);
  archive_read_support_format_raw(mtree);
  if (archive_read_open_filename(mtree, path, 64) != ARCHIVE_OK) {
    archive_read_free(mtree);
    return NULL;
  }

  if (archive_read_next_header(mtree, &entry) != ARCHIVE_OK) {
    archive_read_free(mtree);
    return NULL;
  }

  while (1) {
    ssize_t size;
    while ((size = archive_read_data(mtree, rbuf, 256)) == ARCHIVE_RETRY);
    if (size < 0) { fclose(fbuf); free(buf); return NULL; }
    if (size == 0) { break; }
    fwrite(rbuf, size, 1, fbuf);
  }
  archive_read_free(mtree);
  fclose(fbuf);

  if ((fbuf = fmemopen(buf, len, "r")) == NULL) {
    free(buf);
    return NULL;
  } else if ((reader = pu_mtree_reader_open_stream(fbuf)) == NULL) {
    free(buf);
    fclose(fbuf);
    return NULL;
  } else {
    reader->_stream_buf = buf;
    reader->_close_stream = 1;
    return reader;
  }
}

static char *_pu_mtree_path(const char *mpath, const char *end) {
  char oct[4], *path = malloc(end - mpath), *c = path;
  if (mpath[0] == '.' && mpath[1] == '/') { mpath += 2; }
  oct[3] = '\0';
  while (mpath < end) {
    if (*mpath == '\\' && mpath + 3 < end) {
      strncpy(oct, mpath + 1, 3);
      *(c++) = (int) strtol(oct, NULL, 8);
      mpath += 4;
    } else {
      *(c++) = *mpath;
      mpath++;
    }
  }
  *c = '\0';
  return path;
}

pu_mtree_t *pu_mtree_reader_next(pu_mtree_reader_t *reader, pu_mtree_t *dest) {
  ssize_t len;
  char *saveptr, *c;
  pu_mtree_t *entry = dest;

  len = getline(&reader->_buf, &reader->_buflen, reader->stream);
  if (len == -1) { reader->eof = feof(reader->stream); return NULL; }
  if (reader->_buf[len - 1] == '\n') { reader->_buf[len - 1] = '\0'; }

  c = reader->_buf;
  while (pu_iscspace((unsigned char)*c)) { c++; }

  if (c[0] == '#') {
    return pu_mtree_reader_next(reader, dest);
  } else if (strncmp(c, "/set ", 5) == 0) {
    entry = &reader->defaults;
    c += 5;
  } else {
    char *sep = c, *path;
    while (*sep && !pu_iscspace((unsigned char)*sep)) { sep++; }
    if ((path = _pu_mtree_path(c, sep)) == NULL) { return NULL; }
    if (entry) {
      free(entry->path);
    } else if ((entry = malloc(sizeof(pu_mtree_t))) == NULL) {
      free(path);
      return NULL;
    }
    memcpy(entry, &reader->defaults, sizeof(pu_mtree_t));
    entry->path = path;
    c = sep;
  }

  for (c = strtok_r(c, " ", &saveptr); c; c = strtok_r(NULL, " ", &saveptr)) {
    char *field = c, *val = strchr(field, '=');
    if (val == NULL) { continue; }
    *(val++) = '\0';
    if (strcmp(field, "type") == 0) {
      strcpy(entry->type, val);
    } else if (strcmp(field, "uid") == 0) {
      entry->uid = atoi(val);
    } else if (strcmp(field, "gid") == 0) {
      entry->gid = atoi(val);
    } else if (strcmp(field, "mode") == 0) {
      entry->mode = strtol(val, NULL, 8);
    } else if (strcmp(field, "size") == 0) {
      entry->size = strtol(val, NULL, 10);
    } else if (strcmp(field, "md5digest") == 0) {
      strcpy(entry->md5digest, val);
    } else if (strcmp(field, "sha256digest") == 0) {
      strcpy(entry->sha256digest, val);
    } else {
      /* ignore unknown fields */
    }
  }

  if (entry == &reader->defaults) {
    return pu_mtree_reader_next(reader, dest);
  }

  return entry;
}
