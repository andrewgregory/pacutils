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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>

#include <alpm.h>

#include "pacutils/config.h"
#include "pacutils/mtree.h"
#include "pacutils/ui.h"
#include "pacutils/util.h"

#ifndef PACUTILS_H
#define PACUTILS_H

char *pu_version(void);
void pu_print_version(const char *progname, const char *progver);

int pu_pathcmp(const char *p1, const char *p2);
alpm_file_t *pu_filelist_contains_path(alpm_filelist_t *files, const char *path);

alpm_pkg_t *pu_find_pkgspec(alpm_handle_t *handle, const char *pkgspec);
void pu_fprint_pkgspec(FILE *stream, alpm_pkg_t *pkg);

int pu_log_command(alpm_handle_t *handle, const char *caller, int argc, char **argv);

#endif /* PACUTILS_H */

/* vim: set ts=2 sw=2 noet: */
