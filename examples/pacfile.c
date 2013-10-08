#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>

#include <pacutils.h>

enum longopt_flags {
	FLAG_CONFIG,
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
	exit(ret);
}

pu_config_t *parse_opts(int argc, char **argv)
{
	char *config_file = "/etc/pacman.conf";
	pu_config_t *config = NULL;
	int c;

	char *short_opts = "";
	struct option long_opts[] = {
		{ "config"       , required_argument , NULL , FLAG_CONFIG       } ,
		{ "dbpath"       , required_argument , NULL , FLAG_DBPATH       } ,
		{ "help"         , no_argument       , NULL , FLAG_HELP         } ,
		{ "root"         , required_argument , NULL , FLAG_ROOT         } ,
		{ "version"      , no_argument       , NULL , FLAG_VERSION      } ,
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

mode_t cmp_mode(mode_t pmode, mode_t fmode)
{
	mode_t mask = 07777;
	mode_t pperm = pmode & mask;
	mode_t fperm = fmode & mask;
	const char *ptype = mode_str(pmode);
	const char *ftype = mode_str(fmode);

	if(pperm == fperm) {
		printf("mode: %o\n", pperm);
	} else {
		printf("mode: %o (%o on filesystem)\n", pperm, fperm);
	}

	if(ptype == ftype) {
		printf("type: %s\n", ptype);
	} else {
		printf("type: %s (%s on filesystem)\n", ptype, ftype);
	}

	return pmode;
}

void cmp_time(const char *label, time_t ptime, time_t ftime)
{
	struct tm ltime;
	char time_buf[26];
	strftime(time_buf, 26, "%F %T", localtime_r(&ptime, &ltime));
	if(ftime == ptime) {
		printf("%s: %s\n", label, time_buf);
	} else {
		printf("%s: %s ", label, time_buf);
		strftime(time_buf, 26, "%F %T", localtime_r(&ftime, &ltime));
		printf("(%s on filesystem)\n", time_buf);
	}
}

void cmp_target(struct archive_entry *entry, const char *path, struct stat *buf)
{
	const char *ptarget = archive_entry_symlink(entry);
	printf("target: %s", ptarget);
	if(buf) {
		char ftarget[PATH_MAX];
		ssize_t len = readlink(path, ftarget, PATH_MAX);
		ftarget[len] = '\0';
		if(strcmp(ptarget, ftarget) != 0) {
			printf(" (%s on filesystem)", ftarget);
		}
	}
	putchar('\n');
}

void cmp_uid(struct archive_entry *entry, struct stat *buf)
{
	uid_t puid = archive_entry_uid(entry);
	struct passwd *pw = getpwuid(puid);
	printf("owner: %d (%s)", puid, pw ? pw->pw_name : "unknown user");
	if(puid != buf->st_uid) {
		pw = getpwuid(buf->st_uid);
		printf(" (%d (%s) on filesystem)",
				buf->st_uid, pw ? pw->pw_name : "unknown_user");
	}
	putchar('\n');
}

void cmp_gid(struct archive_entry *entry, struct stat *buf)
{
	gid_t pgid = archive_entry_gid(entry);
	struct group *gr = getgrgid(pgid);
	printf("group: %d (%s)", pgid, gr ? gr->gr_name : "unknown group");
	if(pgid != buf->st_gid) {
		gr = getgrgid(buf->st_gid);
		printf(" (%d (%s) on filesystem)",
				buf->st_gid, gr ? gr->gr_name : "unknown_group");
	}
	putchar('\n');
}

void cmp_size(struct archive_entry *entry, struct stat *buf)
{
	/* FIXME directories always show a discrepancy */
	size_t psize = archive_entry_size(entry);
	if(psize == buf->st_size) {
		printf("size: %d\n", psize);
	} else {
		printf("size: %d (%d on filesystem)\n", psize, buf->st_size);
	}
}

int main(int argc, char **argv)
{
	pu_config_t *config = NULL;
	alpm_handle_t *handle = NULL;
	int ret = 0;
	size_t rootlen;

	if(!(config = parse_opts(argc, argv))) {
		goto cleanup;
	}

	if(!(handle = pu_initialize_handle_from_config(config))) {
		fprintf(stderr, "error: failed to initialize alpm.\n");
		ret = 1;
		goto cleanup;
	}

	rootlen = strlen(alpm_option_get_root(handle));

	for(; optind < argc; optind++) {
		const char *filename = argv[optind];
		const char *relfname = filename + rootlen;
		int found = 0;
		alpm_list_t *p;

		for(p = alpm_db_get_pkgcache(alpm_get_localdb(handle)); p; p = p->next) {
			if(alpm_filelist_contains(alpm_pkg_get_files(p->data), relfname)) {
				alpm_list_t *b;

				if(found) putchar('\n');
				printf("file: %s\n", filename);
				printf("owner: %s\n", alpm_pkg_get_name(p->data));

				found = 1;

				/* FIXME run this once per file, not once per package */
				if(access(filename, R_OK) != 0) {
					fprintf(stderr, "warning: could not read '%s' (%s)\n", filename, strerror(errno));
					continue;
				}

				/* backup file status */
				for(b = alpm_pkg_get_backup(p->data); b; b = b->next) {
					alpm_backup_t *bak = b->data;
					if(strcmp(relfname, bak->name) == 0) {
						char *md5sum = alpm_compute_md5sum(filename);
						if(!md5sum) {
							fprintf(stderr, "warning: could not calculate md5sum for '%s'\n", filename);
						} else {
							if(strcmp(md5sum, bak->hash) == 0) {
								fputs("backup: yes (unmodified)\n", stdout);
							} else {
								fputs("backup: yes (modified)\n", stdout);
							}
							free(md5sum);
						}
						break;
					}
				}
				if(!b) {
					fputs("backup: no\n", stdout);
				}

				/* MTREE info */
				/* FIXME if file doesn't exist only show mtree info */
				struct archive *mtree = alpm_pkg_mtree_open(p->data);
				if(mtree) {
					size_t len = strlen(relfname);
					while(relfname[len - 1] == '/') len--;

					struct archive_entry *entry;
					while(alpm_pkg_mtree_next(p->data, mtree, &entry) == ARCHIVE_OK) {
						struct stat sbuf;

						const char *ppath = archive_entry_pathname(entry);
						if(strncmp("./", ppath, 2) == 0) ppath += 2;

						if(strncmp(relfname, ppath, len) != 0) continue;

						if(lstat(filename, &sbuf) != 0) {
							fprintf(stderr, "warning: could not stat '%s' (%s)\n",
									filename, strerror(errno));
							continue;
						}

						if(S_ISLNK(cmp_mode(archive_entry_mode(entry), sbuf.st_mode))) {
							cmp_target(entry, filename, &sbuf);
						}
						cmp_time("mtime", archive_entry_mtime(entry), sbuf.st_mtime);
						cmp_uid(entry, &sbuf);
						cmp_gid(entry, &sbuf);
						cmp_size(entry, &sbuf);

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
