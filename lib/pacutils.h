#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>

#include <alpm.h>

#include "pacutils/config.h"
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

char *pu_basename(char *path);

#endif /* PACUTILS_H */

/* vim: set ts=2 sw=2 noet: */
