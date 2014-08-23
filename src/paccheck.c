#include <getopt.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#include <pacutils.h>

const char *myname = "paccheck", *myver = "0.1.0";

enum longopt_flags {
	FLAG_ADD = 1000,
	FLAG_BACKUP,
	FLAG_CONFIG,
	FLAG_DBPATH,
	FLAG_DEBUG,
	FLAG_DEPENDS,
	FLAG_FILES,
	FLAG_FILE_PROPERTIES,
	FLAG_HELP,
	FLAG_OPT_DEPENDS,
	FLAG_RECURSIVE,
	FLAG_ROOT,
	FLAG_VERBOSE,
	FLAG_VERSION,
};

enum check_types {
	CHECK_DEPENDS = 1 << 0,
	CHECK_OPT_DEPENDS = 1 << 1,
	CHECK_FILES = 1 << 2,
	CHECK_FILE_PROPERTIES = 1 << 3,
};

pu_config_t *config = NULL;
alpm_handle_t *handle = NULL;
alpm_db_t *localdb = NULL;
alpm_list_t *pkgcache = NULL, *packages = NULL;
int checks = 0, recursive = 0, debug = 0, include_backups = 0, verbose = 0;

void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
#define hputs(s) fputs(s"\n", stream);
	hputs("paccheck - check installed packages");
	hputs("usage:  paccheck [options] [<package>]...");
	hputs("        paccheck (--help|--version)");
	hputs("");
	hputs("   --config=<path>    set an alternate configuration file");
	hputs("   --dbpath=<path>    set an alternate database location");
	hputs("   --root=<path>      set an alternate installation root");
	hputs("   --verbose          display additional information");
	hputs("   --debug            display additional debugging information");
	hputs("   --help             display this help information");
	hputs("   --version          display version information");
	hputs("");
	hputs("   --recursive        perform checks on package [opt-]depends");
	hputs("   --depends          check missing dependencies");
	hputs("   --opt-depends      check missing optional dependencies");
	hputs("   --files            check installed files against package database");
	hputs("   --file-properties  check installed files against MTREE data");
	hputs("   --backup           include backup files in modification checks");
#undef hputs
	exit(ret);
}

pu_config_t *parse_opts(int argc, char **argv)
{
	char *config_file = "/etc/pacman.conf";
	pu_config_t *config = NULL;
	int c;

	char *short_opts = "";
	struct option long_opts[] = {
		{ "config"        , required_argument , NULL       , FLAG_CONFIG       } ,
		{ "dbpath"        , required_argument , NULL       , FLAG_DBPATH       } ,
		{ "root"          , required_argument , NULL       , FLAG_ROOT         } ,
		{ "debug"         , no_argument       , NULL       , FLAG_DEBUG        } ,
		{ "verbose"       , no_argument       , NULL       , FLAG_VERBOSE      } ,

		{ "help"          , no_argument       , NULL       , FLAG_HELP         } ,
		{ "version"       , no_argument       , NULL       , FLAG_VERSION      } ,

		{ "recursive"     , no_argument       , NULL       , FLAG_RECURSIVE    } ,
		{ "backup"        , no_argument       , NULL       , FLAG_BACKUP       } ,

		{ "depends"       , no_argument       , NULL       , FLAG_DEPENDS      } ,
		{ "opt-depends"   , no_argument       , NULL       , FLAG_OPT_DEPENDS  } ,
		{ "files"         , no_argument       , NULL       , FLAG_FILES        } ,
		{ "file-properties", no_argument      , NULL       , FLAG_FILE_PROPERTIES } ,

		{ 0, 0, 0, 0 },
	};

	/* check for a custom config file location */
	while((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
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
		}
	}

	/* load the config file */
	config = pu_config_new_from_file(config_file);
	if(!config) {
		fprintf(stderr, "error: could not parse '%s'\n", config_file);
		return NULL;
	}

	/* process remaining command-line options */
	optind = 1;
	while((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch(c) {

			case 0:
				/* already handled */
				break;

			/* general options */
			case FLAG_CONFIG:
				/* already handled */
				break;
			case FLAG_DBPATH:
				free(config->dbpath);
				config->dbpath = strdup(optarg);
				break;
			case FLAG_DEBUG:
				debug = 1;
				break;
			case FLAG_VERBOSE:
				verbose = 1;
				break;
			case FLAG_ROOT:
				free(config->rootdir);
				config->rootdir = strdup(optarg);
				break;

			/* checks */
			case FLAG_DEPENDS:
				checks |= CHECK_DEPENDS;
				break;
			case FLAG_OPT_DEPENDS:
				checks |= CHECK_OPT_DEPENDS;
				break;
			case FLAG_FILES:
				checks |= CHECK_FILES;
				break;
			case FLAG_FILE_PROPERTIES:
				checks |= CHECK_FILE_PROPERTIES;
				break;

			/* misc */
			case FLAG_RECURSIVE:
				recursive = 1;
				break;
			case FLAG_BACKUP:
				include_backups = 1;
				break;

			case '?':
			default:
				usage(1);
				break;
		}
	}

	return config;
}

static int check_depends(alpm_pkg_t *p)
{
	int ret = 0;
	alpm_list_t *i;
	for(i = alpm_pkg_get_depends(p); i; i = alpm_list_next(i)) {
		char *depstring = alpm_dep_compute_string(i->data);
		if(!alpm_find_satisfier(pkgcache, depstring)) {
			printf("%s: unsatisfied dependency '%s'\n",
					alpm_pkg_get_name(p), depstring);
			ret = 1;
		}
		free(depstring);
	}
	if(verbose && !ret) {
		printf("%s: all dependencies satisfied\n", alpm_pkg_get_name(p));
	}
	return ret;
}

static int check_opt_depends(alpm_pkg_t *p)
{
	int ret = 0;
	alpm_list_t *i;
	for(i = alpm_pkg_get_optdepends(p); i; i = alpm_list_next(i)) {
		char *depstring = alpm_dep_compute_string(i->data);
		if(!alpm_find_satisfier(pkgcache, depstring)) {
			printf("%s: unsatisfied optional dependency '%s'\n",
					alpm_pkg_get_name(p), depstring);
			ret = 1;
		}
		free(depstring);
		i = alpm_list_next(i);
	}
	if(verbose && !ret) {
		printf("%s: all optional dependencies satisfied\n",
				alpm_pkg_get_name(p));
	}
	return ret;
}

/* verify that the filesystem matches the package database */
static int check_files(alpm_pkg_t *pkg)
{
	alpm_filelist_t *filelist = alpm_pkg_get_files(pkg);
	char path[PATH_MAX], *rel;
	unsigned int i;
	int ret = 0;
	size_t space;

	strncpy(path, alpm_option_get_root(handle), PATH_MAX);
	rel = path + strlen(path);
	space = PATH_MAX - (rel - path);

	for(i = 0; i < filelist->count; ++i) {
		alpm_file_t file = filelist->files[i];
		int isdir = 0;
		size_t len;
		struct stat buf;

		strncpy(rel, file.name, space);
		len = strlen(file.name);
		if(rel[len - 1] == '/') {
			isdir = 1;
			rel[len - 1] = '\0';
		}

		if(lstat(path, &buf) != 0) {
			printf("%s: could not locate '%s' (%s)\n",
					alpm_pkg_get_name(pkg), path, strerror(errno));
			ret = 1;
		} else if(isdir && !S_ISDIR(buf.st_mode)) {
			printf("%s: '%s' type mismatch (expected directory)\n",
					alpm_pkg_get_name(pkg), path);
			ret = 1;
		} else if(!isdir && S_ISDIR(buf.st_mode)) {
			printf("%s: '%s' type mismatch (expected file)\n",
					alpm_pkg_get_name(pkg), path);
			ret = 1;
		}
	}

	if(verbose && !ret) {
		printf("%s: all files match database\n", alpm_pkg_get_name(pkg));
	}

	return ret;
}

const char *mode_str(mode_t mode)
{
	if(S_ISREG(mode)) {
		return "file";
	} else if(S_ISDIR(mode)) {
		return "directory";
	} else if(S_ISBLK(mode)) {
		return "block";
	} else if(S_ISCHR(mode)) {
		return "character special file";
	} else if(S_ISFIFO(mode)) {
		return "fifo";
	} else if(S_ISLNK(mode)) {
		return "link";
	} else if(S_ISSOCK(mode)) {
		return "socket";
	} else {
		return "unknown type";
	}
}

int cmp_mode(alpm_pkg_t *pkg, const char *path,
		struct archive_entry *entry, struct stat *st)
{
	mode_t mask = 07777;
	mode_t pmode = archive_entry_mode(entry);
	mode_t perm = pmode & mask;
	const char *type = mode_str(pmode);
	const char *ftype = mode_str(st->st_mode);

	if(perm != (st->st_mode & mask)) {
		printf("%s: '%s' permission mismatch (expected %o)\n",
				alpm_pkg_get_name(pkg), path, perm);
	}

	if(type != ftype) {
		printf("%s: '%s' type mismatch (expected %s)\n",
				alpm_pkg_get_name(pkg), path, type);
		return 1;
	}

	return 0;
}

int cmp_mtime(alpm_pkg_t *pkg, const char *path,
		struct archive_entry *entry, struct stat *st)
{
	time_t t = archive_entry_mtime(entry);

	if(t != st->st_mtime) {
		struct tm ltime;
		char time_buf[26];
		strftime(time_buf, 26, "%F %T", localtime_r(&t, &ltime));
		printf("%s: '%s' modification time mismatch (expected %s)\n",
				alpm_pkg_get_name(pkg), path, time_buf);
		return 1;
	}

	return 0;
}

int cmp_target(alpm_pkg_t *pkg, const char *path,
		struct archive_entry *entry)
{
	const char *ptarget = archive_entry_symlink(entry);
	char ftarget[PATH_MAX];
	ssize_t len = readlink(path, ftarget, PATH_MAX);
	ftarget[len] = '\0';

	if(strcmp(ptarget, ftarget) != 0) {
		printf("%s: '%s' symlink target mismatch (expected %s)\n",
				alpm_pkg_get_name(pkg), path, ptarget);
		return 1;
	}

	return 0;
}

int cmp_uid(alpm_pkg_t *pkg, const char *path,
		struct archive_entry *entry, struct stat *st)
{
	uid_t puid = archive_entry_uid(entry);

	if(puid != st->st_uid) {
		struct passwd *pw = getpwuid(puid);
		printf("%s: '%s' UID mismatch (expected %d/%s)\n",
				alpm_pkg_get_name(pkg), path, puid, pw ? pw->pw_name : "unknown user");
		return 1;
	}

	return 0;
}

int cmp_gid(alpm_pkg_t *pkg, const char *path,
		struct archive_entry *entry, struct stat *st)
{
	gid_t pgid = archive_entry_gid(entry);

	if(pgid != st->st_gid) {
		struct group *gr = getgrgid(pgid);
		printf("%s: '%s' GID mismatch (expected %d/%s)\n",
				alpm_pkg_get_name(pkg), path, pgid, gr ? gr->gr_name : "unknown group");
		return 1;
	}

	return 0;
}

int cmp_size(alpm_pkg_t *pkg, const char *path,
		struct archive_entry *entry, struct stat *st)
{
	int64_t psize = archive_entry_size(entry);

	if(psize != st->st_size) {
		char hr_size[20];
		printf("%s: '%s' size mismatch (expected %s)\n",
				alpm_pkg_get_name(pkg), path, pu_hr_size(psize, hr_size));
		return 1;
	}

	return 0;
}

/* check filesystem against extra mtree data if available,
 * NOT guaranteed to catch db/filesystem discrepencies */
static int check_file_properties(alpm_pkg_t *pkg)
{
	char path[PATH_MAX], *rel;
	int ret = 0;
	size_t space;
	struct archive *mtree = alpm_pkg_mtree_open(pkg);
	struct archive_entry *entry;

	if(!mtree) {
		/* TODO should this return failure? files haven't actually been verified */
		return 0;
	}

	strncpy(path, alpm_option_get_root(handle), PATH_MAX);
	rel = path + strlen(path);
	space = PATH_MAX - (rel - path);

	while(alpm_pkg_mtree_next(pkg, mtree, &entry) == ARCHIVE_OK) {
		const char *ppath = archive_entry_pathname(entry);
		struct stat buf;
		alpm_list_t *i;
		int isbackup = 0;

		if(strncmp("./", ppath, 2) == 0) { ppath += 2; }

		if(strcmp(ppath, ".INSTALL") == 0) {
			/* TODO */
			continue;
		} else if(strcmp(ppath, ".CHANGELOG") == 0) {
			/* TODO */
			continue;
		} else if(ppath[0] == '.') {
			continue;
		} else {
			strncpy(rel, ppath, space);
		}

		if(lstat(path, &buf) != 0) {
			printf("%s: could not locate '%s' (%s)\n",
					alpm_pkg_get_name(pkg), path, strerror(errno));
			ret = 1;
			continue;
		}


		if(!include_backups) {
			for(i = alpm_pkg_get_backup(pkg); i; i = alpm_list_next(i)) {
				alpm_backup_t *b = i->data;
				if(strcmp(ppath, b->name) == 0) {
					isbackup = 1;
					break;
				}
			}
		}

		if(cmp_mode(pkg, path, entry, &buf) != 0) { ret = 1; }
		if(S_ISLNK(buf.st_mode) && S_ISLNK(archive_entry_mode(entry))) {
			if(cmp_target(pkg, path, entry) != 0) { ret = 1; }
		}
		if(!isbackup && !S_ISDIR(buf.st_mode)) {
			/* these are expected to vary for backup files */
			if(cmp_mtime(pkg, path, entry, &buf) != 0) { ret = 1; }
			if(!S_ISLNK(buf.st_mode)) {
				/* always fails for directories and symlinks */
				if(cmp_size(pkg, path, entry, &buf) != 0) { ret = 1; }
			}
		}
		if(cmp_uid(pkg, path, entry, &buf) != 0) { ret = 1; }
		if(cmp_gid(pkg, path, entry, &buf) != 0) { ret = 1; }
	}

	if(verbose && !ret) {
		printf("%s: all files match mtree\n", alpm_pkg_get_name(pkg));
	}

	return ret;
}

int list_find(alpm_list_t *haystack, void *needle)
{
	alpm_list_t *i;
	for(i = haystack; i; i = alpm_list_next(i)) {
		if(i->data == needle) {
			return 1;
		}
	}
	return 0;
}

alpm_list_t *list_add_unique(alpm_list_t *list, void *data)
{
	if(list_find(list, data)) {
		return list; 
	} else {
		return alpm_list_add(list, data);
	}
}

alpm_pkg_t *load_pkg(const char *pkgname)
{
	/* TODO pkgspec handling */
	if(strncmp(pkgname, "local/", 6) == 0) {
		pkgname += 6;
	}
	alpm_pkg_t *p = alpm_db_get_pkg(localdb, pkgname);
	if(p == NULL) {
		fprintf(stderr, "error: could not find package '%s'\n", pkgname);
	} else {
		packages = list_add_unique(packages, p);
	}
	return p;
}

void add_deps(alpm_pkg_t *pkg)
{
	alpm_list_t *i;
	for(i = alpm_pkg_get_depends(pkg); i; i = alpm_list_next(i)) {
		char *depstring = alpm_dep_compute_string(i->data);
		alpm_pkg_t *p = alpm_find_satisfier(pkgcache, depstring);
		if(p && !list_find(packages, p)) {
			packages = alpm_list_add(packages, p);
			add_deps(p);
		}
		free(depstring);
	}
	if(checks & CHECK_OPT_DEPENDS) {
		for(i = alpm_pkg_get_depends(pkg); i; i = alpm_list_next(i)) {
			char *depstring = alpm_dep_compute_string(i->data);
			alpm_pkg_t *p = alpm_find_satisfier(pkgcache, depstring);
			if(p && !list_find(packages, p)) {
				packages = alpm_list_add(packages, p);
				add_deps(p);
			}
			free(depstring);
		}
	}
}

int main(int argc, char **argv)
{
	alpm_list_t *i;
	int ret = 0;

	if(!(config = parse_opts(argc, argv))) {
		ret = 1;
		goto cleanup;
	}

	if(checks == 0) {
		checks = CHECK_DEPENDS | CHECK_FILES;
	}

	if(!(handle = pu_initialize_handle_from_config(config))) {
		fprintf(stderr, "error: failed to initialize alpm.\n");
		ret = 1;
		goto cleanup;
	}

	localdb = alpm_get_localdb(handle);
	pkgcache = alpm_db_get_pkgcache(localdb);

	for(; optind < argc; ++optind) {
		if(load_pkg(argv[optind]) == NULL) { ret = 1; }
	}
	if(!isatty(fileno(stdin)) && !feof(stdin)) {
		char buf[256];
		while(fgets(buf, 256, stdin)) {
			size_t len = strlen(buf);
			if(buf[len - 1] == '\n') { buf[len - 1] = '\0'; }
			if(load_pkg(buf) == NULL) { ret = 1; }
		}
	}

	if(ret) { goto cleanup; }

	if(packages == NULL) {
		packages = alpm_list_copy(pkgcache);
		recursive = 0;
	} else if(recursive) {
		/* load [opt-]depends */
		alpm_list_t *i, *originals = alpm_list_copy(packages);
		for(i = originals; i; i = alpm_list_next(i)) {
			add_deps(i->data);
		}
		alpm_list_free(originals);
	}

	for(i = packages; i; i = alpm_list_next(i)) {
		if(debug) { printf("debug: checking '%s'\n", alpm_pkg_get_name(i->data)); }

#define RUNCHECK(t, b) if((checks & t) && b != 0) { ret = 1; }
		RUNCHECK(CHECK_DEPENDS, check_depends(i->data));
		RUNCHECK(CHECK_OPT_DEPENDS, check_opt_depends(i->data));
		RUNCHECK(CHECK_FILES, check_files(i->data));
		RUNCHECK(CHECK_FILE_PROPERTIES, check_file_properties(i->data));
#undef RUNCHECK
	}

cleanup:
	alpm_list_free(packages);
	alpm_release(handle);
	pu_config_free(config);

	return ret;
}

/* vim: set ts=2 sw=2 noet: */