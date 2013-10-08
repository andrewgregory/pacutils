#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>

#include <pacutils.h>

int checkfs = 1;

enum longopt_flags {
	FLAG_CONFIG = 1000,
	FLAG_DBPATH,
	FLAG_HELP,
	FLAG_ROOT,
	FLAG_VERSION,
};

void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
	fputs("pacfile - display information about pacman-managed files\n", stream);
	fputs("usage:  pacfile [options] <file>...\n", stream);
	fputs("        pacfile (--help|--version)\n", stream);
	fputs("options:\n", stream);
	fputs("   --config=<path>    set an alternate configuration file\n", stream);
	fputs("   --dbpath=<path>    set an alternate database location\n", stream);
	fputs("   --root=<path>      set an alternate installation root\n", stream);
	fputs("   --help             display this help information\n", stream);
	fputs("   --version          display version information\n", stream);
	fputs("   --no-check         do not compare pkg values to fs\n", stream);
	exit(ret);
}

pu_config_t *parse_opts(int argc, char **argv)
{
	char *config_file = "/etc/pacman.conf";
	pu_config_t *config = NULL;
	int c;

	char *short_opts = "";
	struct option long_opts[] = {
		{ "config"       , required_argument , NULL       , FLAG_CONFIG       } ,
		{ "dbpath"       , required_argument , NULL       , FLAG_DBPATH       } ,
		{ "help"         , no_argument       , NULL       , FLAG_HELP         } ,
		{ "root"         , required_argument , NULL       , FLAG_ROOT         } ,
		{ "version"      , no_argument       , NULL       , FLAG_VERSION      } ,
		{ "no-check"     , no_argument       , &checkfs   , 0                 } ,
		{ 0, 0, 0, 0 },
	};

	/* check for a custom config file location */
	opterr = 0;
	c = getopt_long(argc, argv, short_opts, long_opts, NULL);
	while(c != -1) {
		if(c == FLAG_CONFIG) {
			config_file = optarg;
			break;
		}
		c = getopt_long(argc, argv, short_opts, long_opts, NULL);
	}

	/* load the config file */
	config = pu_config_new_from_file(config_file);
	if(!config) {
		fprintf(stderr, "error: could not parse '%s'\n", config_file);
		return NULL;
	}

	/* process remaining command-line options */
	optind = opterr = 1;
	c = getopt_long(argc, argv, short_opts, long_opts, NULL);
	while(c != -1) {
		switch(c) {
			case 0:
			case FLAG_CONFIG:
				/* already handled */
				break;
			case FLAG_DBPATH:
				free(config->dbpath);
				config->dbpath = strdup(optarg);
				break;
			case FLAG_HELP:
				usage(0);
				break;
			case FLAG_ROOT:
				free(config->rootdir);
				config->rootdir = strdup(optarg);
				break;
			case FLAG_VERSION:
				pu_print_version("pacfile", "0.1");
				exit(0);
				break;
			case '?':
			default:
				usage(1);
				break;
		}
		c = getopt_long(argc, argv, short_opts, long_opts, NULL);
	}

	return config;
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

mode_t cmp_mode(struct archive_entry *entry, struct stat *st)
{
	mode_t mask = 07777;
	mode_t pmode = archive_entry_mode(entry);
	mode_t perm = pmode & mask;
	const char *type = mode_str(pmode);


	printf("mode:   %o", perm);
	if(st && perm != (st->st_mode & mask)) {
		printf(" (%o on filesystem)", st->st_mode & mask);
	}
	putchar('\n');

	printf("type:   %s", type);
	if(st) {
		const char *ftype = mode_str(st->st_mode);
		if(strcmp(type, ftype) != 0) {
			printf(" (%s on filesystem)", ftype);
		}
	}
	putchar('\n');

	return pmode;
}

void cmp_mtime(struct archive_entry *entry, struct stat *st)
{
	struct tm ltime;
	char time_buf[26];

	time_t t = archive_entry_mtime(entry);
	strftime(time_buf, 26, "%F %T", localtime_r(&t, &ltime));
	printf("mtime:  %s", time_buf);

	if(st && t != st->st_mtime) {
		strftime(time_buf, 26, "%F %T", localtime_r(&st->st_mtime, &ltime));
		printf("(%s on filesystem)", time_buf);
	}

	putchar('\n');
}

void cmp_target(struct archive_entry *entry, struct stat *st, const char *path)
{
	const char *ptarget = archive_entry_symlink(entry);
	printf("target: %s", ptarget);

	if(st) {
		char ftarget[PATH_MAX];
		ssize_t len = readlink(path, ftarget, PATH_MAX);
		ftarget[len] = '\0';
		if(strcmp(ptarget, ftarget) != 0) {
			printf(" (%s on filesystem)", ftarget);
		}
	}

	putchar('\n');
}

void cmp_uid(struct archive_entry *entry, struct stat *st)
{
	uid_t puid = archive_entry_uid(entry);
	struct passwd *pw = getpwuid(puid);

	printf("owner:  %d (%s)", puid, pw ? pw->pw_name : "unknown user");

	if(st && puid != st->st_uid) {
		pw = getpwuid(st->st_uid);
		printf(" (%d (%s) on filesystem)",
				st->st_uid, pw ? pw->pw_name : "unknown_user");
	}

	putchar('\n');
}

void cmp_gid(struct archive_entry *entry, struct stat *st)
{
	gid_t pgid = archive_entry_gid(entry);
	struct group *gr = getgrgid(pgid);

	printf("group:  %d (%s)", pgid, gr ? gr->gr_name : "unknown group");

	if(st && pgid != st->st_gid) {
		gr = getgrgid(st->st_gid);
		printf(" (%d (%s) on filesystem)",
				st->st_gid, gr ? gr->gr_name : "unknown_group");
	}

	putchar('\n');
}

void cmp_size(struct archive_entry *entry, struct stat *st)
{
	/* FIXME directories and symlinks always show a discrepancy */
	size_t psize = archive_entry_size(entry);

	printf("size:   %zd", psize);

	if(st && psize != st->st_size) {
		printf(" (%zd on filesystem)", st->st_size);
	}

	putchar('\n');
}

int main(int argc, char **argv)
{
	pu_config_t *config = NULL;
	alpm_handle_t *handle = NULL;
	int ret = 0;
	size_t rootlen;
	const char *root;

	if(!(config = parse_opts(argc, argv))) {
		goto cleanup;
	}

	if(!(handle = pu_initialize_handle_from_config(config))) {
		fprintf(stderr, "error: failed to initialize alpm.\n");
		ret = 1;
		goto cleanup;
	}

	root = alpm_option_get_root(handle);
	rootlen = strlen(root);

	for(; optind < argc; optind++) {
		const char *relfname, *filename = argv[optind];

		int found = 0;
		alpm_list_t *p;

		if(strncmp(filename, root, rootlen) == 0) {
			relfname = filename + rootlen;
		} else {
			relfname = filename;
		}

		for(p = alpm_db_get_pkgcache(alpm_get_localdb(handle)); p; p = p->next) {
			alpm_file_t *pfile = pu_filelist_contains_path(
					alpm_pkg_get_files(p->data), relfname);
			if(pfile) {
				alpm_list_t *b;
				char full_path[PATH_MAX];
				snprintf(full_path, PATH_MAX, "%s%s", root, pfile->name);

				if(found++) putchar('\n');

				printf("file:   %s\n", pfile->name);
				printf("owner:  %s\n", alpm_pkg_get_name(p->data));

				/* backup file status */
				for(b = alpm_pkg_get_backup(p->data); b; b = b->next) {
					alpm_backup_t *bak = b->data;
					if(pu_pathcmp(relfname, bak->name) == 0) {
						fputs("backup: yes", stdout);

						if(checkfs) {
							char *md5sum = alpm_compute_md5sum(full_path);
							if(!md5sum) {
								fprintf(stderr, "warning: could not calculate md5sum for '%s'\n",
										full_path);
								ret = 1;
							} else {
								if(strcmp(md5sum, bak->hash) == 0) {
									fputs(" (unmodified)", stdout);
								} else {
									fputs(" (modified)", stdout);
								}
								free(md5sum);
							}
						}

						putchar('\n');
						break;
					}
				}
				if(!b) {
					fputs("backup: no\n", stdout);
				}

				/* MTREE info */
				struct archive *mtree = alpm_pkg_mtree_open(p->data);
				if(mtree) {

					struct archive_entry *entry;
					while(alpm_pkg_mtree_next(p->data, mtree, &entry) == ARCHIVE_OK) {
						struct stat sbuf, *st = NULL;

						const char *ppath = archive_entry_pathname(entry);
						if(strncmp("./", ppath, 2) == 0) ppath += 2;

						if(pu_pathcmp(relfname, ppath) != 0) continue;

						if(checkfs) {
							if(lstat(full_path, &sbuf) != 0) {
								fprintf(stderr, "warning: could not stat '%s' (%s)\n",
										full_path, strerror(errno));
								ret = 1;
							} else {
								st = &sbuf;
							}
						}

						if(S_ISLNK(cmp_mode(entry, st))) {
							cmp_target(entry, st, full_path);
						}
						cmp_mtime(entry, st);
						cmp_uid(entry, st);
						cmp_gid(entry, st);
						cmp_size(entry, st);

						break;
					}
					alpm_pkg_mtree_close(p->data, mtree);
				}
			}
		}

		if(!found) {
			printf("no package owns '%s'\n", filename);
		}
	}

cleanup:
	alpm_release(handle);
	pu_config_free(config);

	return ret;
}

/* vim: set ts=2 sw=2 noet: */
