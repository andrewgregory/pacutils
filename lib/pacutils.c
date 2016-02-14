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

#include "pacutils.h"

#include <glob.h>
#include <sys/time.h>
#include <sys/utsname.h>

char *pu_version(void)
{
	return "0.1";
}

void pu_print_version(const char *progname, const char *progver)
{
	printf("%s v%s - libalpm v%s - pacutils v%s\n", progname, progver,
			alpm_version(), pu_version());
}

int pu_pathcmp(const char *p1, const char *p2)
{
	while(*p1 && *p1 == *p2) {
		/* skip repeated '/' */
		if(*p1 == '/') {
			while(*(++p1) == '/');
			while(*(++p2) == '/');
		} else {
			p1++;
			p2++;
		}
	}

	/* skip trailing '/' */
	if(*p1 == '\0') {
		while(*p2 == '/') p2++;
	} else if(*p2 == '\0') {
		while(*p1 == '/') p1++;
	}

	return *p1 - *p2;
}

static int _pu_filelist_path_cmp(const char *needle, alpm_file_t *file)
{
	return pu_pathcmp(needle, file->name);
}

alpm_file_t *pu_filelist_contains_path(alpm_filelist_t *files, const char *path)
{
	if(files == NULL) { return NULL; }

	return bsearch(path, files->files, files->count, sizeof(alpm_file_t),
			(int(*)(const void*, const void*)) _pu_filelist_path_cmp);
}

alpm_pkg_t *pu_find_pkgspec(alpm_handle_t *handle, const char *pkgspec)
{
	char *c;

	if(strstr(pkgspec, "://")) {
		alpm_pkg_t *pkg;
		alpm_siglevel_t sl
			= strncmp(pkgspec, "file://", 7) == 0
			? alpm_option_get_local_file_siglevel(handle)
			: alpm_option_get_remote_file_siglevel(handle);
		char *path = alpm_fetch_pkgurl(handle, pkgspec);
		int err = alpm_pkg_load(handle, path ? path : pkgspec, 1, sl, &pkg);
		free(path);
		if(!err) {
			return pkg;
		}
	} else if((c = strchr(pkgspec, '/')))  {
		alpm_db_t *db = NULL;
		size_t dblen = c - pkgspec;

		if(dblen == strlen("local") && strncmp(pkgspec, "local", dblen) == 0) {
			db = alpm_get_localdb(handle);
		} else {
			alpm_list_t *i;
			for(i = alpm_get_syncdbs(handle); i; i = i->next) {
				const char *dbname = alpm_db_get_name(i->data);
				if(dblen == strlen(dbname) && strncmp(pkgspec, dbname, dblen) == 0) {
					db = i->data;
					break;
				}
			}
		}

		if(!db) {
			return NULL;
		} else {
			return alpm_db_get_pkg(db, c + 1);
		}
	}

	return NULL;
}

void pu_fprint_pkgspec(FILE *stream, alpm_pkg_t *pkg)
{
	const char *c;
	switch(alpm_pkg_get_origin(pkg)) {
		case ALPM_PKG_FROM_FILE:
			c = alpm_pkg_get_filename(pkg);
			if(strstr(c, "://")) {
				fprintf(stream, "%s", alpm_pkg_get_filename(pkg));
			} else {
				char *real = realpath(c, NULL);
				fprintf(stream, "file://%s", real);
				free(real);
			}
			break;
		case ALPM_PKG_FROM_LOCALDB:
			fprintf(stream, "local/%s", alpm_pkg_get_name(pkg));
			break;
		case ALPM_PKG_FROM_SYNCDB:
			fprintf(stream, "%s/%s",
					alpm_db_get_name(alpm_pkg_get_db(pkg)), alpm_pkg_get_name(pkg));
			break;
		default:
			/* no idea where this package came from, fall back to its name */
			fputs(alpm_pkg_get_name(pkg), stream);
			break;
	}
}

int pu_log_command(alpm_handle_t *handle, const char *caller, int argc, char **argv)
{
	int i;
	char *cmd;
	size_t cmdlen = 0;

	for(i = 0; i < argc; ++i) {
		cmdlen += strlen(argv[i]) + 1;
	}

	cmd = malloc(cmdlen + 1);
	if(!cmd) {
		return -1;
	}

	for(i = 0; i < argc; ++i) {
		strcat(cmd, " ");
		strcat(cmd, argv[i]);
	}

	alpm_logaction(handle, caller, "Running%s\n", cmd);

	free(cmd);

	return 0;
}

/* vim: set ts=2 sw=2 noet: */
