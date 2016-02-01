#include <getopt.h>
#include <errno.h>
#include <stdio.h>

#include <pacutils.h>

const char *myname = "pacinfo", *myver = "0.1";

pu_config_t *config = NULL;
alpm_handle_t *handle = NULL;

int level = 2, removable_size = 0, raw = 0;
int isep = '\n';
char *dbext = NULL;

enum longopt_flags {
	FLAG_CONFIG = 1000,
	FLAG_DBEXT,
	FLAG_DBPATH,
	FLAG_DEBUG,
	FLAG_HELP,
	FLAG_NULL,
	FLAG_REMOVABLE,
	FLAG_ROOT,
	FLAG_VERSION,
};

void printt(const char *field, alpm_time_t t) {
		char time_buf[50];
		struct tm ltime;
		if(t) {
			if(raw) {
				snprintf(time_buf, 50, "%lu", t);
			} else {
				strftime(time_buf, 50, "%F %T", localtime_r(&t, &ltime));
			}
			printf(field, time_buf);
		}
}

void printl(const char *field, alpm_list_t *values) {
	while(values) {
		printf(field, values->data);
		values = values->next;
	}
}

void printd(const char *field, alpm_list_t *values) {
	while(values) {
		char *s = alpm_dep_compute_string(values->data);
		printf(field, s);
		free(s);
		values = values->next;
	}
}

void printo(const char *field, off_t size) {
	char hrsize[50];
	if(raw) {
		snprintf(hrsize, 50, "%lu", size);
	} else {
		pu_hr_size(size, hrsize);
	}
	printf(field, hrsize);
}

off_t _pkg_removable_size(alpm_pkg_t *pkg, alpm_list_t *pkgs, alpm_list_t **seen)
{
	const char *pkgname = alpm_pkg_get_name(pkg);
	if(alpm_list_find_ptr(*seen, pkgname))
		return 0;

	off_t size = alpm_pkg_get_isize(pkg);
	*seen = alpm_list_add(*seen,(void*) pkgname);

	alpm_list_t *d, *deps = alpm_pkg_get_depends(pkg);
	for(d = deps; d; d = d->next) {
		char *depstring = alpm_dep_compute_string(d->data);
		alpm_pkg_t *p = alpm_find_satisfier(pkgs, depstring);
		free(depstring);

		if(!p) {
			continue;
		}

		if(alpm_pkg_get_reason(p) == ALPM_PKG_REASON_EXPLICIT) {
			continue;
		}

		if(alpm_list_find_ptr(*seen, p)) {
			continue;
		}

		alpm_list_t *rb = alpm_pkg_compute_requiredby(p);

		if(rb) {
			alpm_list_t *diff = alpm_list_diff(rb, *seen,
					(alpm_list_fn_cmp) strcmp);
			if(diff) {
				alpm_list_free(diff);
				FREELIST(rb);
				continue;
			}
			FREELIST(rb);
		}

		size += _pkg_removable_size(p, pkgs, seen);
	}

	return size;
}

off_t pkg_removable_size(alpm_handle_t *handle, alpm_pkg_t *pkg) {
	alpm_db_t *localdb = alpm_get_localdb(handle);
	alpm_list_t *seen = NULL, *installed = alpm_db_get_pkgcache(localdb);
	alpm_list_free(seen);
	return _pkg_removable_size(pkg, installed, &seen);
}

void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
#define hputs(s) fputs(s"\n", stream);
	hputs("pacinfo - display package information");
	hputs("usage:  pacinfo [options] <pkgspec>...");
	hputs("        pacinfo (--help|--version)");
	hputs("");
	hputs("options:");
	hputs("   --cachedir=<path>  set an alternate cache location");
	hputs("   --config=<path>    set an alternate configuration file");
	hputs("   --dbext=<ext>      set an alternate sync database extension");
	hputs("   --dbpath=<path>    set an alternate database location");
	hputs("   --debug            enable extra debugging messages");
	hputs("   --null[=sep]       parse stdin as <sep> separated values (default NUL)");
	hputs("   --short            display brief package information");
	hputs("   --raw              display raw numeric values");
	hputs("   --root=<path>      set an alternate installation root");
	hputs("   --removable-size   include removable dependencies in size");
	hputs("   --help             display this help information");
	hputs("   --version          display version information");
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
		{ "help"          , no_argument       , NULL       , FLAG_HELP         } ,
		{ "version"       , no_argument       , NULL       , FLAG_VERSION      } ,

		{ "dbext"         , required_argument , NULL       , FLAG_DBEXT        } ,
		{ "dbpath"        , required_argument , NULL       , FLAG_DBPATH       } ,
		{ "debug"         , no_argument       , NULL       , FLAG_DEBUG        } ,
		{ "root"          , required_argument , NULL       , FLAG_ROOT         } ,
		{ "null"          , optional_argument , NULL       , FLAG_NULL         } ,

		{ "short"         , no_argument       , &level     , 1                 } ,
		{ "raw"           , no_argument       , &raw       , 1                 } ,

		{ "removable-size", no_argument       , NULL       , FLAG_REMOVABLE    } ,
		{ 0, 0, 0, 0 },
	};

	if((config = pu_config_new()) == NULL) {
		perror("malloc");
		return NULL;
	}

	while((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch(c) {

			case 0:
				/* already handled */
				break;

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

			case FLAG_DBEXT:
				dbext = optarg;
				break;
			case FLAG_DBPATH:
				free(config->dbpath);
				config->dbpath = strdup(optarg);
				break;
			case FLAG_DEBUG:
				break;
			case FLAG_REMOVABLE:
				removable_size = 1;
				break;
			case FLAG_ROOT:
				free(config->rootdir);
				config->rootdir = strdup(optarg);
				break;
			case FLAG_NULL:
				isep = optarg ? optarg[0] : '\0';
				break;
			case '?':
				usage(1);
				break;
		}
	}

	if(!pu_ui_config_load(config, config_file)) {
		fprintf(stderr, "error: could not parse '%s'\n", config_file);
		pu_config_free(config);
		return NULL;
	}

	return config;
}

void print_pkg_info(alpm_pkg_t *pkg) {
		alpm_db_t *db = alpm_pkg_get_db(pkg);
		alpm_db_t *localdb = alpm_get_localdb(handle);

		switch(level) {
			case 1:
				printf("%s/%s %s", alpm_db_get_name(db), alpm_pkg_get_name(pkg),
						alpm_pkg_get_version(pkg));
				{
					alpm_list_t *g = alpm_pkg_get_groups(pkg);
					if(g) {
						fputs(" (", stdout);
						for(; g; g = g->next) {
							fputs(g->data, stdout);
							if(g->next) {
								fputc(' ', stdout);
							}
						}
						fputc(')', stdout);
					}
				}
				if(db != localdb) {
					alpm_pkg_t *lpkg = alpm_db_get_pkg(localdb, alpm_pkg_get_name(pkg));
					if(lpkg) {
						const char *lver = alpm_pkg_get_version(lpkg);
						if(strcmp(lver, alpm_pkg_get_version(pkg)) == 0) {
							fputs(" [installed]", stdout);
						} else {
							printf(" [installed: %s]", lver);
						}
					}
				}
				fputs("\n    ", stdout);
				fputs(alpm_pkg_get_desc(pkg), stdout);
				break;
			case 2:
				printf("Name:           %s\n", alpm_pkg_get_name(pkg));
				printf("Base:           %s\n", alpm_pkg_get_base(pkg));
				printf("Repository:     %s\n", alpm_db_get_name(db));
				printf("File:           %s\n", alpm_pkg_get_filename(pkg));
				printf("Version:        %s\n", alpm_pkg_get_version(pkg));
				printf("Description:    %s\n", alpm_pkg_get_desc(pkg));
				printf("Architecture:   %s\n", alpm_pkg_get_arch(pkg));
				printf("URL:            %s\n", alpm_pkg_get_url(pkg));
				printl("Licenses:       %s\n", alpm_pkg_get_licenses(pkg));
				printl("Groups:         %s\n", alpm_pkg_get_groups(pkg));
				printd("Provides:       %s\n", alpm_pkg_get_provides(pkg));
				printd("Requires:       %s\n", alpm_pkg_get_depends(pkg));
				printd("Conflicts:      %s\n", alpm_pkg_get_conflicts(pkg));
				printd("Replaces:       %s\n", alpm_pkg_get_replaces(pkg));
				printo("Package Size:   %s\n", alpm_pkg_get_size(pkg));
				printo("Download Size:  %s\n", alpm_pkg_download_size(pkg));
				printo("Installed Size: %s\n",
						removable_size
						? pkg_removable_size(handle, pkg)
						: alpm_pkg_get_isize(pkg));
				printf("Packager:       %s\n", alpm_pkg_get_packager(pkg));
				printt("Build Date:     %s\n", alpm_pkg_get_builddate(pkg));
				printt("Install Date:   %s\n", alpm_pkg_get_installdate(pkg));
				printf("MD5 Sum:        %s\n", alpm_pkg_get_md5sum(pkg));
				printf("SHA-256 Sum:    %s\n", alpm_pkg_get_sha256sum(pkg));

				/*printd("Required For:   ",      alpm_pkg_get_provides(pkg));*/
				/*printd("Optional For:   ",      alpm_pkg_get_provides(pkg));*/
				/* install reason */
				/* install script */
				/* validated by */
				break;
		}

		putchar('\n');
}

alpm_list_t *find_pkg(const char *pkgspec) {
	alpm_list_t *i, *pkgs = NULL;
	alpm_pkg_t *pkg = pu_find_pkgspec(handle, pkgspec);

	if(pkg) {
		return alpm_list_add(NULL, pkg);
	}

	if((pkg = alpm_db_get_pkg(alpm_get_localdb(handle), pkgspec))) {
		pkgs = alpm_list_add(pkgs, pkg);
	}

	for(i = alpm_get_syncdbs(handle); i; i = alpm_list_next(i)) {
		if((pkg = alpm_db_get_pkg(i->data, pkgspec))) {
			pkgs = alpm_list_add(pkgs, pkg);
		}
	}

	return pkgs;
}

void print_pkgspec_info(const char *pkgspec) {
	alpm_list_t *i, *pkgs = find_pkg(pkgspec);
	if(!pkgs) {
		fprintf(stderr, "Unable to find package '%s'\n", pkgspec);
		return;
	}
	for(i = pkgs; i; i = alpm_list_next(i)) {
		print_pkg_info(i->data);
	}
	alpm_list_free(pkgs);
}

int main(int argc, char **argv) {
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

	for(argv += optind; *argv; ++argv) {
		print_pkgspec_info(*argv);
	}

	if(!isatty(fileno(stdin)) && errno != EBADF) {
		char *buf = NULL;
		size_t len = 0;
		ssize_t read;
		while((read = getdelim(&buf, &len, isep, stdin)) != -1) {
			if(buf[read - 1] == isep) { buf[read - 1] = '\0'; }
			print_pkgspec_info(buf);
		}
		free(buf);
	}

cleanup:
	alpm_release(handle);
	pu_config_free(config);

	return ret;
}

/* vim: set ts=2 sw=2 noet: */
