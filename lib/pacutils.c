#include "pacutils.h"

#include <glob.h>
#include <sys/time.h>
#include <sys/utsname.h>

#define PU_MAX_REFRESH_MS 200

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

static int _pu_filelist_path_cmp(alpm_file_t *f1, alpm_file_t *f2)
{
	return pu_pathcmp(f1->name, f2->name);
}

alpm_file_t *pu_filelist_contains_path(alpm_filelist_t *files, const char *path)
{
	alpm_file_t needle;

	if(files == NULL) {
		return NULL;
	}

	needle.name = (char*) path;

	return bsearch(&needle, files->files, files->count, sizeof(alpm_file_t),
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

/**
 * @brief print unique identifier for a package
 *
 * @param pkg
 */
void pu_fprint_pkgspec(FILE *stream, alpm_pkg_t *pkg)
{
	const char *c;
	switch(alpm_pkg_get_origin(pkg)) {
		case ALPM_PKG_FROM_FILE:
			c = alpm_pkg_get_filename(pkg);
			if(strstr(c, "://")) {
				fprintf(stream, "%s", alpm_pkg_get_filename(pkg));
			} else {
				c = realpath(c, NULL);
				fprintf(stream, "file://%s", c);
				free((char*) c);
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

void pu_display_transaction(alpm_handle_t *handle)
{
	off_t install = 0, download = 0, delta = 0;
	char size[20];
	alpm_db_t *ldb = alpm_get_localdb(handle);
	alpm_list_t *i;

	for(i = alpm_trans_get_remove(handle); i; i = i->next) {
		alpm_pkg_t *p = i->data;
		printf("removing %s/%s (%s)\n",
				alpm_db_get_name(alpm_pkg_get_db(p)),
				alpm_pkg_get_name(p),
				alpm_pkg_get_version(p));

		install -= alpm_pkg_get_isize(i->data);
		delta -= alpm_pkg_get_isize(i->data);
	}

	for(i = alpm_trans_get_add(handle); i; i = i->next) {
		alpm_pkg_t *p = i->data;
		alpm_pkg_t *lpkg = alpm_db_get_pkg(ldb, alpm_pkg_get_name(p));

		switch(alpm_pkg_get_origin(p)) {
			case ALPM_PKG_FROM_FILE:
				printf("installing %s (%s)",
						alpm_pkg_get_filename(p),
						alpm_pkg_get_name(p));
				break;
			case ALPM_PKG_FROM_SYNCDB:
				printf("installing %s/%s",
						alpm_db_get_name(alpm_pkg_get_db(p)),
						alpm_pkg_get_name(p));
				break;
			case ALPM_PKG_FROM_LOCALDB:
				/* impossible */
				break;
		}

		if(lpkg) {
			printf(" (%s -> %s)\n",
					alpm_pkg_get_version(lpkg), alpm_pkg_get_version(p));
		} else {
			printf(" (%s)\n", alpm_pkg_get_version(p));
		}

		install  += alpm_pkg_get_isize(p);
		download += alpm_pkg_download_size(p);
		delta    += alpm_pkg_get_isize(p);
		if(lpkg) {
			delta  -= alpm_pkg_get_isize(lpkg);
		}
	}

	fputs("\n", stdout);
	printf("Download Size:  %10s\n", pu_hr_size(download, size));
	printf("Installed Size: %10s\n", pu_hr_size(install, size));
	printf("Size Delta:     %10s\n", pu_hr_size(delta, size));
}

int pu_log_command(alpm_handle_t *handle, const char *caller, int argc, char **argv)
{
	int i;
	char *cmd;
	size_t cmdlen = strlen("Running");

	for(i = 0; i < argc; ++i) {
		cmdlen += strlen(argv[i]) + 1;
	}

	cmd = malloc(cmdlen + 2);
	if(!cmd) {
		return -1;
	}

	strcpy(cmd, "Running");
	for(i = 0; i < argc; ++i) {
		strcat(cmd, " ");
		strcat(cmd, argv[i]);
	}
	strcat(cmd, "\n");

	alpm_logaction(handle, caller, cmd);

	free(cmd);

	return 0;
}

char *pu_basename(char *path)
{
	char *c;

	if(!path) {
		return NULL;
	}

	for(c = path + strlen(path); c > path && *(c - 1) != '/'; --c) {
		/* empty loop */
	}

	return c;
}

/* vim: set ts=2 sw=2 noet: */
