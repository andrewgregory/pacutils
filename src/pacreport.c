/*
 * Copyright 2013-2016 Andrew Gregory <andrew.gregory.8@gmail.com>
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

#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>
#include <math.h>
#include <fnmatch.h>
#include <fcntl.h>

#include <pacutils.h>

#include "config-defaults.h"

#include "../ext/mini.c/mini.c"

const char *myname = "pacreport", *myver = BUILDVER;

pu_config_t *config = NULL;
alpm_handle_t *handle;
alpm_list_t *groups = NULL, *ignore = NULL, *pkg_ignore = NULL;
int missing_files = 0, backup_files = 0, orphan_files = 0;
char *dbext = NULL;
const char *sysroot = NULL;

enum longopt_flags {
	FLAG_BACKUPS = 1000,
	FLAG_CACHEDIR,
	FLAG_CONFIG,
	FLAG_DBEXT,
	FLAG_DBPATH,
	FLAG_GROUP,
	FLAG_HELP,
	FLAG_MISSING_FILES,
	FLAG_ORPHANS,
	FLAG_ROOT,
	FLAG_SYSROOT,
	FLAG_VERSION,
};

struct pkg_ignore_t {
	char *pkgname;
	char *ignore;
};

struct pkg_file_t {
	alpm_pkg_t *pkg;
	alpm_file_t *file;
};

static int streq(const char *s1, const char *s2)
{
	return (s1 == s2 || (s1 && s2 && strcmp(s1, s2) == 0));
}

static void warn(const char *fmt, ...)
{
	if(fmt) {
		va_list ap;
		va_start(ap, fmt);
		fputs("warning: ", stderr);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}

static void die(const char *fmt, ...)
{
	if(fmt) {
		va_list ap;
		va_start(ap, fmt);
		fputs("error: ", stderr);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
	exit(1);
}

struct pkg_ignore_t *pkg_ignore_new(const char *pkgname, const char *ignore)
{
	struct pkg_ignore_t *pi = malloc(sizeof(struct pkg_ignore_t));
	pi->pkgname = strdup(pkgname);
	pi->ignore = strdup(ignore);
	return pi;
}

void pkg_ignore_free(struct pkg_ignore_t *pi)
{
	if(pi) {
		free(pi->pkgname);
		free(pi->ignore);
		free(pi);
	}
}

struct pkg_file_t *pkg_file_new(alpm_pkg_t *pkg, alpm_file_t *file)
{
	struct pkg_file_t *mf = calloc(sizeof(struct pkg_file_t), 1);
	if(!mf) {
		return NULL;
	}
	mf->pkg = pkg;
	mf->file = file;
	return mf;
}

/**
 * @brief Calculates the total size of all unneeded dependencies of a package.
 *
 * @param handle
 * @param pkg
 * @param depchain list of already processed packages
 *
 * @return size in bytes
 */
off_t get_pkg_chain_size(alpm_handle_t *handle, alpm_pkg_t *pkg)
{
	alpm_db_t *localdb = alpm_get_localdb(handle);
	alpm_list_t *localpkgs = alpm_db_get_pkgcache(localdb);
	alpm_list_t *depchain = alpm_list_add(NULL, pkg);
	off_t size = 0;
	alpm_list_t *d;


	for(d = depchain; d; d = d->next) {
		alpm_pkg_t *p = d->data;
		alpm_list_t *dep, *deps = alpm_pkg_get_depends(p);

		size += alpm_pkg_get_isize(p);

		for(dep = deps; dep; dep = dep->next) {
			char *depstring = alpm_dep_compute_string(dep->data);
			alpm_pkg_t *satisfier = alpm_find_satisfier(localpkgs, depstring);
			free(depstring);

			/* move on if the dependency was installed explicitly or already
			 * processed */
			if(!satisfier
					|| alpm_pkg_get_reason(satisfier) == ALPM_PKG_REASON_EXPLICIT
					|| alpm_list_find_ptr(depchain, satisfier)) {
				continue;
			}

			/* check if the dependency is required outside the chain */
			alpm_list_t *r, *rb = alpm_pkg_compute_requiredby(satisfier);
			int required = 0;
			for(r = rb; r; r = r->next) {
				alpm_pkg_t *p = alpm_db_get_pkg(localdb, r->data);
				if(!alpm_list_find_ptr(depchain, p)) {
					required = 1;
					break;
				}
			}
			FREELIST(rb);

			if(!required) {
				depchain = alpm_list_add(depchain, satisfier);
			}
		}
	}

	alpm_list_free(depchain);
	return size;
}

void print_pkg_info(alpm_handle_t *handle, alpm_pkg_t *pkg, size_t pkgname_len)
{
	char size[20];
	alpm_list_t *group;

	printf("  %-*s	%8s", (int) pkgname_len, alpm_pkg_get_name(pkg),
			pu_hr_size( get_pkg_chain_size(handle, pkg), size));

	if(alpm_pkg_get_groups(pkg)) {
		fputs(" (", stdout);
		for(group = alpm_pkg_get_groups(pkg); group; group = group->next) {
			fputs(group->data, stdout);
			if(group->next) {
				putchar(' ');
			}
		}
		putchar(')');
	}

	putchar('\n');
}

void print_pkglist(alpm_handle_t *handle, alpm_list_t *pkgs)
{
	size_t pkgname_len = 0;
	alpm_list_t *p;
	for(p = pkgs; p; p = p->next) {
		size_t len = strlen(alpm_pkg_get_name(p->data));
		if(len > pkgname_len) {
			pkgname_len = len;
		}
	}
	for(p = pkgs; p; p = p->next) {
		print_pkg_info(handle, p->data, pkgname_len);
	}
}

void print_toplevel_explicit(alpm_handle_t *handle)
{
	alpm_db_t *localdb = alpm_get_localdb(handle);
	alpm_list_t *matches = NULL, *p, *pkgs = alpm_db_get_pkgcache(localdb);

	for(p = pkgs; p; p = p->next) {
		alpm_list_t *rb = alpm_pkg_compute_requiredby(p->data);
		if(!rb && alpm_pkg_get_reason(p->data) == ALPM_PKG_REASON_EXPLICIT) {
			matches = alpm_list_add(matches, p->data);
		}
		FREELIST(rb);
	}
	printf("Unneeded Packages Installed Explicitly:\n");
	print_pkglist(handle, matches);
	alpm_list_free(matches);
}

void print_toplevel_depends(alpm_handle_t *handle)
{
	alpm_db_t *localdb = alpm_get_localdb(handle);
	alpm_list_t *matches = NULL, *p, *pkgs = alpm_db_get_pkgcache(localdb);

	for(p = pkgs; p; p = p->next) {
		alpm_list_t *rb = alpm_pkg_compute_requiredby(p->data);
		if(!rb && alpm_pkg_get_reason(p->data) == ALPM_PKG_REASON_DEPEND) {
			matches = alpm_list_add(matches, p->data);
		}
		FREELIST(rb);
	}
	printf("Unneeded Packages Installed As Dependencies:\n");
	print_pkglist(handle, matches);
	alpm_list_free(matches);
}

int pkg_is_foreign(alpm_handle_t *handle, alpm_pkg_t *pkg)
{
	alpm_list_t *s;
	const char *pkgname = alpm_pkg_get_name(pkg);
	for(s = alpm_get_syncdbs(handle); s; s = s->next) {
		if(alpm_db_get_pkg(s->data, pkgname)) {
			return 0;
		}
	}
	return 1;
}

void print_foreign(alpm_handle_t *handle)
{
	alpm_db_t *localdb = alpm_get_localdb(handle);
	alpm_list_t *matches = NULL, *p, *pkgs = alpm_db_get_pkgcache(localdb);

	for(p = pkgs; p; p = p->next) {
		if(pkg_is_foreign(handle, p->data)) {
			matches = alpm_list_add(matches, p->data);
		}
	}
	printf("Installed Packages Not In A Repository:\n");
	print_pkglist(handle, matches);
	alpm_list_free(matches);
}

void print_group_missing(alpm_handle_t *handle, alpm_list_t *groups)
{
	alpm_db_t *localdb = alpm_get_localdb(handle);
	alpm_list_t *localpkgs = alpm_db_get_pkgcache(localdb);
	alpm_list_t *matches = NULL;
	alpm_list_t *i;

	for(i = groups; i; i = i->next) {
		const char *group = i->data;
		alpm_list_t *p, *pkgs;
		pkgs = alpm_find_group_pkgs(alpm_get_syncdbs(handle), group);
		for(p = pkgs; p; p = p->next) {
			const char *pkgname = alpm_pkg_get_name(p->data);
			if(!alpm_list_find_ptr(matches, p->data)
					&& !alpm_find_satisfier(localpkgs, pkgname)) {
				matches = alpm_list_add(matches, p->data);
			}
		}
		alpm_list_free(pkgs);
	}

	puts("Missing Group Packages:");
	print_pkglist(handle, matches);
	alpm_list_free(matches);
}

void print_filelist(alpm_handle_t *handle, alpm_list_t *files)
{
	size_t pkgname_len = 0;
	alpm_list_t *f;
	const char *root = alpm_option_get_root(handle);
	for(f = files; f; f = f->next) {
		struct pkg_file_t *mf = f->data;
		size_t len = strlen(alpm_pkg_get_name(mf->pkg));
		if(len > pkgname_len) {
			pkgname_len = len;
		}
	}
	for(f = files; f; f = f->next) {
		struct pkg_file_t *mf = f->data;
		printf("  %-*s	%s%s\n", (int) pkgname_len, alpm_pkg_get_name(mf->pkg),
				root, mf->file->name);
	}
}

void print_missing_files(alpm_handle_t *handle)
{
	alpm_db_t *localdb = alpm_get_localdb(handle);
	alpm_list_t *matches = NULL, *p, *pkgs = alpm_db_get_pkgcache(localdb);
	char path[PATH_MAX], *tail;
	strncpy(path, alpm_option_get_root(handle), PATH_MAX);
	size_t len = strlen(path);
	size_t max = PATH_MAX - len;
	tail = path + len;

	for(p = pkgs; p; p = p->next) {
		alpm_filelist_t *files = alpm_pkg_get_files(p->data);
		size_t i;
		for(i = 0; i < files->count; ++i) {
			strncpy(tail, files->files[i].name, max);
			if(access(path, F_OK) != 0) {
				struct pkg_file_t *mf = pkg_file_new(p->data, &files->files[i]);
				matches = alpm_list_add(matches, mf);
			}
		}
	}
	puts("Missing Package Files:");
	print_filelist(handle, matches);
	FREELIST(matches);
}

int is_cache_file_installed(alpm_handle_t *handle, const char *path)
{
	char *pkgver, *pkgname = strdup(path);
	int ret = 0;

	if(!(pkgver = strrchr(pkgname, '-'))) {
		goto cleanup;
	}
	*pkgver = '\0';
	for(pkgver--; *pkgver != '-' && pkgver > pkgname; pkgver--);
	for(pkgver--; *pkgver != '-' && pkgver > pkgname; pkgver--);

	if(pkgver > pkgname) {
		*pkgver = '\0';
		pkgver++;
		alpm_pkg_t *lp = alpm_db_get_pkg(alpm_get_localdb(handle), pkgname);
		if(lp && alpm_pkg_vercmp(alpm_pkg_get_version(lp), pkgver) == 0) {
			ret = 1;
		}
	}

cleanup:
	free(pkgname);
	return ret;
}

off_t get_cache_size(alpm_handle_t *handle, int fd, const char *path,
		off_t *uninstalled)
{
	off_t bytes = 0;
	struct dirent *de;
	int dirfd;
	DIR *d;

	if((dirfd = openat(fd, path, O_RDONLY | O_DIRECTORY)) < 0) {
		warn("unable to open cachedir '%s' (%s)\n", path, strerror(errno));
		return 0;
	}
	if((d = fdopendir(dirfd)) == NULL) {
		warn("unable to open cachedir '%s' (%s)\n", path, strerror(errno));
		close(dirfd);
		return 0;
	}

	for(de = readdir(d); de; de = readdir(d)) {
		struct stat buf;
		if(de->d_name[0] == '.'
				&& (de->d_name[1] == '\0' || strcmp("..", de->d_name) == 0)) {
			continue;
		}
		if(fstatat(dirfd, de->d_name, &buf, AT_SYMLINK_NOFOLLOW) != 0) {
			warn("unable to stat '%s' (%s)\n", de->d_name, strerror(errno));
			continue;
		}

		if(S_ISDIR(buf.st_mode)) {
			bytes += get_cache_size(handle, dirfd, de->d_name, uninstalled);
		} else {
			bytes += buf.st_size;
			if(uninstalled && !is_cache_file_installed(handle, de->d_name)) {
				*uninstalled += buf.st_size;
			}
		}
	}

	closedir(d);
	return bytes;
}

void print_cache_sizes(alpm_handle_t *handle)
{
	alpm_list_t *c, *cache_dirs = alpm_option_get_cachedirs(handle);
	size_t pathlen = 0;

	for(c = cache_dirs; c; c = c->next) {
		size_t len = strlen(c->data);
		if(len > pathlen) {
			pathlen = len;
		}
	}

	puts("Package Cache Size:");
	for(c = cache_dirs; c; c = c->next) {
		off_t uninstalled = 0;
		char size[10], usize[10];
		pu_hr_size(get_cache_size(handle, AT_FDCWD, c->data, &uninstalled), size);
		pu_hr_size(uninstalled, usize);
		printf("  %*s %s (%s not installed)\n",
				(int) pathlen, (char*) c->data, size, usize);
	}
}

int should_ignore_file(alpm_handle_t *handle, const char *path)
{
	const char *root = alpm_option_get_root(handle);
	size_t rootlen = strlen(root);
	alpm_db_t *ldb = alpm_get_localdb(handle);
	alpm_list_t *p;

	for(p = ignore; p; p = p->next) {
		if(fnmatch(p->data, path + rootlen, 0) == 0) {
			return 1;
		}
	}
	for(p = pkg_ignore; p; p = p->next) {
		struct pkg_ignore_t *pi = p->data;
		alpm_pkg_t *pkg = alpm_db_get_pkg(ldb, pi->pkgname);
		if(pkg && fnmatch(pi->ignore, path + rootlen, 0) == 0) {
			return 1;
		}
	}
	return 0;
}

int file_is_unowned(alpm_handle_t *handle, const char *path)
{
	alpm_db_t *ldb = alpm_get_localdb(handle);
	alpm_list_t *p, *pkgs = alpm_db_get_pkgcache(ldb);
	for(p = pkgs; p; p = p->next) {
		if(alpm_filelist_contains(alpm_pkg_get_files(p->data), path + 1)) {
			return 0;
		}
	}
	return 1;
}

void _scan_filesystem(alpm_handle_t *handle, const char *dir, int backups,
		int orphans, alpm_list_t **backups_found, alpm_list_t **orphans_found)
{
	static char *skip[] = {
		"/etc/ssl/certs",
		"/dev",
		"/home",
		"/media",
		"/mnt",
		"/proc",
		"/root",
		"/run",
		"/sys",
		"/tmp",
		"/usr/share/mime",
		"/var/cache",
		"/var/log",
		"/var/run",
		"/var/tmp",
		NULL
	};

	char path[PATH_MAX];
	char *filename = path + strlen(dir);
	strcpy(path, dir);

	DIR *dirp = opendir(dir);
	if(!dirp) {
		fprintf(stderr, "Error opening '%s' (%s).\n", dir, strerror(errno));
		return;
	}

	struct dirent *entry;
	while((entry = readdir(dirp))) {
		struct stat buf;
		char **s;
		int need_skip = 0;

		if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 ) {
			continue;
		}

		strcpy(filename, entry->d_name);

		for(s = skip; *s && !need_skip; s++) {
			if(strcmp(path, *s) == 0) {
				need_skip = 1;
			}
		}
		if(need_skip || should_ignore_file(handle, path)) {
			continue;
		}

		if(lstat(path, &buf) != 0) {
			fprintf(stderr, "Error reading '%s' (%s).\n", path, strerror(errno));
			continue;
		}

		if(S_ISDIR(buf.st_mode)) {
			strcat(filename, "/");
			if(orphans && file_is_unowned(handle, path)) {
				*orphans_found = alpm_list_add(*orphans_found, strdup(path));
				if(backups) {
					_scan_filesystem(handle, path, backups, 0, backups_found, orphans_found);
				}
			} else {
				_scan_filesystem(handle, path, backups, orphans, backups_found, orphans_found);
			}
		} else {
			if(orphans && file_is_unowned(handle, path)) {
				*orphans_found = alpm_list_add(*orphans_found, strdup(path));
			}

			if(backups) {
				if(strstr(filename, ".pacnew")
						|| strstr(filename, ".pacsave")
						|| strstr(filename, ".pacorig")) {
					*backups_found = alpm_list_add(*backups_found, strdup(path));
				}
			}
		}
	}
	closedir(dirp);
}

void scan_filesystem(alpm_handle_t *handle, int backups, int orphans)
{
	char *base_dir = "/etc/";
	alpm_list_t *orphans_found = NULL, *backups_found = NULL;
	if(backups > 1 || orphans) {
		base_dir = "/";
	}
	_scan_filesystem(handle, base_dir, backups, orphans, &backups_found, &orphans_found);

	if(orphans) {
		puts("Unowned Files:");
		if(!orphans_found) {
			puts("  None");
		} else {
			alpm_list_t *i;
			orphans_found = alpm_list_msort(orphans_found,
					alpm_list_count(orphans_found), (alpm_list_fn_cmp) strcmp);
			for(i = orphans_found; i; i = i->next) {
				printf("  %s\n", (char*) i->data);
			}
			FREELIST(orphans_found);
		}
	}

	if(backups) {
		puts("Pacman Backup Files:");
		if(!backups_found) {
			puts("  None");
		} else {
			alpm_list_t *i;
			backups_found = alpm_list_msort(backups_found,
					alpm_list_count(backups_found), (alpm_list_fn_cmp) strcmp);
			for(i = backups_found; i; i = i->next) {
				printf("  %s\n", (char*) i->data);
			}
			FREELIST(backups_found);
		}
	}
}

void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
#define hputs(s) fputs(s"\n", stream);
	hputs("pacreport - generate installed package report");
	hputs("usage: pacreport [options]");
	hputs("       pacreport (--help|--version)");
	hputs("");
	hputs("options:");
	hputs("   --root=<path>      set an alternate installation root");
	hputs("   --sysroot=<path>   set an alternate system root");
	hputs("   --cachedir=<path>  set an alternate cache location");
	hputs("   --config=<path>    set an alternate pacman configuration file");
	hputs("   --dbext=<ext>      set an alternate sync database extension");
	hputs("   --dbpath=<path>    set an alternate database location");
	hputs("");
	hputs("   --backups          list .pac{save,orig,new} files");
	hputs("                      (pass twice for extended search outside /etc)");
	hputs("   --group=<GROUP>    list missing group packages");
	hputs("   --missing-files    list missing package files");
	hputs("   --unowned-files    list unowned files");
	hputs("   --help             display this help information");
	hputs("   --version          display version information");
#undef hputs
	exit(ret);
}

pu_config_t *parse_opts(int argc, char **argv)
{
	char *config_file = PACMANCONF;
	int c;

	char *short_opts = "";

	struct option long_opts[] = {
		{ "cachedir"      , required_argument , NULL       , FLAG_CACHEDIR      } ,
		{ "config"        , required_argument , NULL       , FLAG_CONFIG        } ,
		{ "dbext"         , required_argument , NULL       , FLAG_DBEXT         } ,
		{ "dbpath"        , required_argument , NULL       , FLAG_DBPATH        } ,
		{ "root"          , required_argument , NULL       , FLAG_ROOT          } ,
		{ "sysroot"       , required_argument , NULL       , FLAG_SYSROOT       } ,

		{"backups"        , no_argument       , NULL       , FLAG_BACKUPS       } ,
		{"group"          , required_argument , NULL       , FLAG_GROUP         } ,
		{"missing-files"  , no_argument       , NULL       , FLAG_MISSING_FILES } ,
		{"unowned-files"  , no_argument       , NULL       , FLAG_ORPHANS       } ,

		{"help"           , no_argument       , NULL       , FLAG_HELP          } ,
		{"version"        , no_argument       , NULL       , FLAG_VERSION       } ,

		{0, 0, 0, 0 }
	};

	if((config = pu_config_new()) == NULL) {
		perror("malloc");
		return NULL;
	}

	while(( c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch(c) {
			case FLAG_CONFIG:
				config_file = optarg;
				break;
			case FLAG_HELP:
				usage(0);
				break;
			case FLAG_VERSION:
				pu_print_version(myname, myver);
				exit(0);
				break;

			case FLAG_BACKUPS:
				++backup_files;
				break;
			case FLAG_GROUP:
				groups = alpm_list_add(groups, strdup(optarg));
				break;
			case FLAG_CACHEDIR:
				FREELIST(config->cachedirs);
				config->cachedirs = alpm_list_add(NULL, strdup(optarg));
				break;
			case FLAG_DBEXT:
				dbext = optarg;
				break;
			case FLAG_DBPATH:
				free(config->dbpath);
				config->dbpath = strdup(optarg);
				break;
			case FLAG_MISSING_FILES:
				++missing_files;
				break;
			case FLAG_ORPHANS:
				++orphan_files;
				break;
			case FLAG_ROOT:
				free(config->rootdir);
				config->rootdir = strdup(optarg);
				break;
			case FLAG_SYSROOT:
				sysroot = optarg;
				break;

			default:
				usage(2);
				break;
		}
	}

	if(!pu_ui_config_load_sysroot(config, config_file, sysroot)) {
		fprintf(stderr, "error: could not parse '%s'\n", config_file);
		return NULL;
	}

	return config;
}

void parse_config(const char *path)
{
	mini_t *mini = mini_init(path);
	if(mini == NULL) {
		if(errno == ENOENT) { return; }
		die("failed to initialize ini parser for '%s' (%s)\n", path, strerror(errno));
	}
	while(mini_next(mini)) {
		if(mini->key) {
			if(!mini->value) {
				die("option '%s' require a value\n", mini->key);
			}
			if(streq(mini->section, "PkgIgnoreUnowned")) {
				struct pkg_ignore_t *pi = pkg_ignore_new(mini->key, mini->value);
				pkg_ignore = alpm_list_add(pkg_ignore, pi);
			} else if(streq(mini->section, "Options")) {
				if(streq(mini->key, "IgnoreUnowned")) {
					ignore = alpm_list_add(ignore, strdup(mini->value));
				} else {
					warn("unknown option '%s' in section '%s'\n", mini->key, mini->section);
				}
			} else {
				warn("unknown option '%s' in section '%s'\n", mini->key, mini->section);
			}
		}
	}
	if(!mini->eof) {
		die("reading '%s' failed (%s)\n", path, strerror(errno));
	}
	mini_free(mini);
}

int main(int argc, char **argv)
{
	int ret = 0;

	if(!(config = parse_opts(argc, argv))) {
		goto cleanup;
	}

	if(!(handle = pu_initialize_handle_from_config(config))) {
		fprintf(stderr, "error: failed to initialize alpm.\n");
		ret = 1;
		goto cleanup;
	}
	if(dbext && alpm_option_set_dbext(handle, dbext) != 0) {
		fprintf(stderr, "error: unable to set database file extension (%s)\n",
				alpm_strerror(alpm_errno(handle)));
		ret = 1;
		goto cleanup;
	}
	pu_register_syncdbs(handle, config->repos);

	parse_config(SYSCONFDIR "/pacreport.conf");

	if(backup_files || orphan_files) {
		scan_filesystem(handle, backup_files, orphan_files);
	}

	print_toplevel_explicit(handle);
	print_toplevel_depends(handle);
	print_foreign(handle);
	if(groups) {
		print_group_missing(handle, groups);
	}

	if(missing_files) {
		print_missing_files(handle);
	}
	print_cache_sizes(handle);

cleanup:
	FREELIST(groups);
	FREELIST(ignore);
	alpm_list_free_inner(pkg_ignore, (alpm_list_fn_free) pkg_ignore_free);
	alpm_list_free(pkg_ignore);
	alpm_release(handle);
	pu_config_free(config);

	return ret;
}

/* vim: set ts=2 sw=2 noet: */
