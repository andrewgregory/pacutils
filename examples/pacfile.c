#include <errno.h>
#include <getopt.h>

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

				found = 1;
				printf("'%s' is owned by %s\n", filename, alpm_pkg_get_name(p->data));

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
								puts("  backup file status: UNMODIFIED");
							} else {
								puts("  backup file status: MODIFIED");
							}
							free(md5sum);
						}
					}
				}

				/* MTREE info */
				struct archive *mtree = alpm_pkg_mtree_open(p->data);
				struct archive_entry *entry;
				while(alpm_pkg_mtree_next(p->data, mtree, &entry) == ARCHIVE_OK) {
					struct stat sbuf;

					const char *ppath = archive_entry_pathname(entry);
					if(strncmp("./", ppath, 2) == 0) ppath += 2;

					if(strcmp(relfname, ppath) != 0) continue;

					if(lstat(filename, &sbuf) != 0) {
						fprintf(stderr, "warning: could not stat '%s' (%s)\n", filename, strerror(errno));
						continue;
					}

					/* TODO show the file/pkg values */
					/* TODO type, permissions, link target, size */

					if(sbuf.st_mtime == archive_entry_mtime(entry)) {
						printf("  modification time MATCH\n", sbuf.st_mtime);
					} else {
						printf("  modification time MISMATCH\n", sbuf.st_mtime);
					}

					break;
				}
				alpm_pkg_mtree_close(p->data, mtree);
			}
		}

		if(!found) {
			printf("'%s' is unowned\n", filename);
		}
	}

cleanup:
	alpm_release(handle);
	pu_config_free(config);

	return ret;
}

/* vim: set ts=2 sw=2 noet: */
