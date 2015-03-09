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

static long _pu_time_diff(struct timeval *t1, struct timeval *t2)
{
	return (t1->tv_sec - t2->tv_sec) * 1000 + (t1->tv_usec - t2->tv_usec) / 1000;
}

void pu_cb_question(alpm_question_t *question)
{
	switch(question->type) {
		case ALPM_QUESTION_INSTALL_IGNOREPKG:
			{
				alpm_question_install_ignorepkg_t *q = &question->install_ignorepkg;
				q->install = pu_confirm(1,
						"Install ignored package '%s'?", alpm_pkg_get_name(q->pkg));
			}
			break;
		case ALPM_QUESTION_REPLACE_PKG:
			{
				alpm_question_replace_t *q = &question->replace;
				q->replace = pu_confirm(1, "Replace '%s' with '%s'?",
						alpm_pkg_get_name(q->oldpkg), alpm_pkg_get_name(q->newpkg));
			}
			break;
		case ALPM_QUESTION_CONFLICT_PKG:
			{
				alpm_question_conflict_t *q = (alpm_question_conflict_t*) question;
				alpm_conflict_t *c = q->conflict;
				q->remove = pu_confirm(1, "'%s' conflicts with '%s'.  Remove '%s'?",
						c->package1, c->package2, c->package2);
			}
			break;
		case ALPM_QUESTION_REMOVE_PKGS:
		case ALPM_QUESTION_SELECT_PROVIDER:
		case ALPM_QUESTION_CORRUPTED_PKG:
		case ALPM_QUESTION_IMPORT_KEY:
		default:
			break;
	}
}

void pu_cb_download(const char *filename, off_t xfered, off_t total)
{
	static struct timeval last_update = {0, 0};
	int percent;

	if(xfered > 0 && xfered < total) {
		struct timeval now;
		gettimeofday(&now, NULL);
		if(_pu_time_diff(&now, &last_update) < PU_MAX_REFRESH_MS) {
			return;
		}
		last_update = now;
	}

	percent = 100 * xfered / total;
	printf("downloading %s (%ld/%ld) %d%%", filename, xfered, total, percent);

	if(xfered == total) {
		putchar('\n');
	} else {
		putchar('\r');
	}

	fflush(stdout);
}

const char *pu_msg_progress(alpm_progress_t event)
{
	switch(event) {
		case ALPM_PROGRESS_ADD_START:
			return "installing";
		case ALPM_PROGRESS_UPGRADE_START:
			return "upgrading";
		case ALPM_PROGRESS_DOWNGRADE_START:
			return "downgrading";
		case ALPM_PROGRESS_REINSTALL_START:
			return "reinstalling";
		case ALPM_PROGRESS_REMOVE_START:
			return "removing";
		case ALPM_PROGRESS_CONFLICTS_START:
			return "checking for file conflicts";
		case ALPM_PROGRESS_DISKSPACE_START:
			return "checking available disk space";
		case ALPM_PROGRESS_INTEGRITY_START:
			return "checking package integrity";
		case ALPM_PROGRESS_KEYRING_START:
			return "checking keys in keyring";
		case ALPM_PROGRESS_LOAD_START:
			return "loading package files";
		default:
			return "working";
	}
}

void pu_cb_progress(alpm_progress_t event, const char *pkgname, int percent,
		size_t total, size_t current)
{
	const char *opr = pu_msg_progress(event);
	static int percent_last = -1;

	/* don't update if nothing has changed */
	if(percent_last == percent) {
		return;
	}

	if(pkgname && pkgname[0]) {
		printf("%s %s (%zd/%zd) %d%%", opr, pkgname, current, total, percent);
	} else {
		printf("%s (%zd/%zd) %d%%", opr, current, total, percent);
	}

	if(percent == 100) {
		putchar('\n');
	} else {
		putchar('\r');
	}

	fflush(stdout);
	percent_last = percent;
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

int pu_confirm(int def, const char *prompt, ...)
{
	va_list args;
	va_start(args, prompt);
	fputs("\n:: ", stdout);
	vprintf(prompt, args);
	fputs(def ? " [Y/n]" : " [y/N]", stdout);
	va_end(args);
	while(1) {
		int c = getchar();
		if(c != '\n') {
			while(getchar() != '\n');
		}

		switch(c) {
			case '\n':
				return def;
			case 'Y':
			case 'y':
				return 1;
			case 'N':
			case 'n':
				return 0;
		}
	}
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
