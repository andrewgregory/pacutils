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

#include <ctype.h>
#include <limits.h>
#include <string.h>

#include <archive.h>

#include "mtree.h"

static char *_slurp_archive_entry(struct archive *a) {
  char *buf, rbuf[256];
  size_t len;
  FILE *fbuf = open_memstream(&buf, &len);
  while(1) {
      ssize_t size;
      while((size = archive_read_data(a, rbuf, 256)) == ARCHIVE_RETRY);
      if(size < 0) { fclose(fbuf); free(buf); return NULL; }
      if(size == 0) { break; }
      fwrite(rbuf, size, 1, fbuf);
  }
  fclose(fbuf);
  return buf;
}

static struct archive *_open_pkg_mtree(alpm_handle_t *h, alpm_pkg_t *p) {
  struct archive *mtree;
  const char *dbpath = alpm_option_get_dbpath(h);
  const char *pkgname = alpm_pkg_get_name(p);
  const char *pkgver = alpm_pkg_get_version(p);
  char path[PATH_MAX];

  sprintf(path, "%slocal/%s-%s/mtree", dbpath, pkgname, pkgver);

  if((mtree = archive_read_new()) == NULL) { return NULL; }
  archive_read_support_filter_all(mtree);
  archive_read_support_format_raw(mtree);
  if(archive_read_open_filename(mtree, path, 64) != ARCHIVE_OK) {
    archive_read_free(mtree);
    return NULL;
  }

  return mtree;
}

alpm_list_t *pu_mtree_load_pkg_mtree(alpm_handle_t *handle, alpm_pkg_t *pkg) {
  alpm_list_t *entries = NULL;
  pu_mtree_t defaults;
  struct archive *mtree = NULL;
  struct archive_entry *entry = NULL;
  char *line = NULL, *buf = NULL;

  memset(&defaults, 0, sizeof(pu_mtree_t));

  if((mtree = _open_pkg_mtree(handle, pkg)) == NULL) { goto done; }
  if(archive_read_next_header(mtree, &entry) != ARCHIVE_OK) { goto done; }
  if((buf = _slurp_archive_entry(mtree)) == NULL) { goto done; }

  char *saveptr;
  for(line = strtok_r(buf, "\n", &saveptr); line; line = strtok_r(NULL, "\n", &saveptr)) {
    pu_mtree_t *entry;
    char *saveptr2;
    char *c = line;
    while(isspace(*c)) { c++; }

    if(c[0] == '#') {
      continue;
    } else if(strncmp(c, "/set ", 5) == 0) {
      entry = &defaults;
      c += 5;
    } else {
      char *sep = c;
      while(*sep && !isspace(*sep)) { sep++; }
      entry = malloc(sizeof(pu_mtree_t));
      memcpy(entry, &defaults, sizeof(pu_mtree_t));
      c+= 2;
      entry->path = strndup(c, sep - c);
      c = sep;
      entries = alpm_list_add(entries, entry);
    }

    for(c = strtok_r(c, " \n", &saveptr2); c; c = strtok_r(NULL, " \n", &saveptr2)) {
      char *field = c, *val = strchr(field, '=');
      if(val == NULL) { continue; }
      *(val++) = '\0';
      if(strcmp(field, "type") == 0) {
        strcpy(entry->type, val);
      } else if(strcmp(field, "uid") == 0) {
        entry->uid = atoi(val);
      } else if(strcmp(field, "gid") == 0) {
        entry->gid = atoi(val);
      } else if(strcmp(field, "mode") == 0) {
        entry->mode = strtol(val, NULL, 8);
      } else if(strcmp(field, "size") == 0) {
        entry->size = strtol(val, NULL, 10);
      } else if(strcmp(field, "md5digest") == 0) {
        strcpy(entry->md5digest, val);
      } else if(strcmp(field, "sha256digest") == 0) {
        strcpy(entry->sha256digest, val);
      } else {
        /* ignore unknown fields */
      }
    }
  }

done:
  archive_read_free(mtree);
  free(buf);

  return entries;
}

void pu_mtree_free(pu_mtree_t *mtree) {
  if(mtree) {
    free(mtree->path);
    free(mtree);
  }
}

/* vim: set ts=2 sw=2 et: */
