#include <getopt.h>

#include <pacutils.h>

const char *myname, *myver = "0.1";
#define LOG_PREFIX "PACTRANS"

pu_config_t *config = NULL;
alpm_handle_t *handle = NULL;

alpm_loglevel_t log_level = ALPM_LOG_ERROR | ALPM_LOG_WARNING;
alpm_transflag_t trans_flags = 0;

alpm_list_t *spec = NULL, *add = NULL, *rem = NULL, *files = NULL;
alpm_list_t **list = &spec;
alpm_list_t *ignore_pkg = NULL, *ignore_group = NULL;
int printonly = 0, noconfirm = 0, sysupgrade = 0, downgrade = 0;

enum longopt_flags {
	FLAG_ADD = 1000,
	FLAG_ASDEPS,
	FLAG_ASEXPLICIT,
	FLAG_CACHEDIR,
	FLAG_CASCADE,
	FLAG_CONFIG,
	FLAG_DBONLY,
	FLAG_DBPATH,
	FLAG_DEBUG,
	FLAG_DLONLY,
	FLAG_DOWNGRADE,
	FLAG_FILE,
	FLAG_HELP,
	FLAG_IGNORE_PKG,
	FLAG_IGNORE_GROUP,
	FLAG_LOGFILE,
	FLAG_NOBACKUP,
	FLAG_NOCONFIRM,
	FLAG_NODEPS,
	FLAG_NOSCRIPTLET,
	FLAG_PRINT,
	FLAG_RECURSIVE,
	FLAG_REMOVE,
	FLAG_ROOT,
	FLAG_SPEC,
	FLAG_SYSUPGRADE,
	FLAG_UNNEEDED,
	FLAG_VERSION,
};

void fatal(const char *msg)
{
	fputs(msg, stderr);
	exit(1);
}

void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
#define hputf(format, ...) fprintf(stream, format"\n", __VA_ARGS__);
#define hputs(s) fputs(s"\n", stream);
	hputf("%s - install/remove packages", myname);
	hputf("usage:  %s [options] ([<action>] <target>...)...", myname);
	hputf("        %s (--help|--version)", myname);
	hputs("");
	hputs("actions (may be used together):");
	hputf("   --spec             install sync/file specs, remove local specs%s", list == &spec ? " (default)" : "");
	hputf("   --install          install packages from sync database%s", list == &add ? " (default)" : "");
	hputf("   --remove           remove packages%s", list == &rem ? " (default)" : "");
	hputs("   --file             install packages from files");
	hputs("   --sysupgrade       upgrade installed packages");
	hputs("");
	hputs("options:");
	hputs("   --cachedir=<path>  set an alternate cache location");
	hputs("   --config=<path>    set an alternate configuration file");
	hputs("   --dbonly");
	hputs("   --dbpath=<path>    set an alternate database location");
	hputs("   --debug            enable extra debugging messages");
	hputs("   --logfile=<path>   set an alternate log file");
	hputs("   --print-only");
	hputs("   --no-confirm");
	hputs("   --no-deps");
	hputs("   --no-scriptlet");
	hputs("   --root=<path>      set an alternate installation root");
	hputs("   --help             display this help information");
	hputs("   --version          display version information");
	hputs("");
	hputs("sysupgrade options:");
	hputs("   --ignore-pkg=<package>");
	hputs("                      ignore upgrades for <package>");
	hputs("   --ignore-group=<group>");
	hputs("                      ignore upgrades for packages in group <group>");
	hputs("");
	hputs("install/file options:");
	hputs("   --as-deps          install packages as dependencies");
	hputs("   --as-explicit      install packages as explicit");
	hputs("   --download-only    download packages without installing");
	hputs("");
	hputs("remove options:");
	hputs("   --cascade");
	hputs("   --no-backup");
	hputs("   --recursive");
	hputs("   --unneeded");
#undef hputs
#undef hputf
	exit(ret);
}

pu_config_t *parse_opts(int argc, char **argv)
{
	char *config_file = "/etc/pacman.conf";
	pu_config_t *config = NULL;
	int c;

	char *short_opts = "-";
	struct option long_opts[] = {
		{ "spec"          , no_argument       , NULL       , FLAG_SPEC         } ,
		{ "install"       , no_argument       , NULL       , FLAG_ADD          } ,
		{ "file"          , no_argument       , NULL       , FLAG_FILE         } ,
		{ "remove"        , no_argument       , NULL       , FLAG_REMOVE       } ,

		{ "cachedir"      , required_argument , NULL       , FLAG_CACHEDIR     } ,
		{ "config"        , required_argument , NULL       , FLAG_CONFIG       } ,
		{ "dbonly"        , no_argument       , NULL       , FLAG_DBONLY       } ,
		{ "dbpath"        , required_argument , NULL       , FLAG_DBPATH       } ,
		{ "debug"         , optional_argument , NULL       , FLAG_DEBUG        } ,
		{ "logfile"       , required_argument , NULL       , FLAG_LOGFILE      } ,
		{ "print-only"    , no_argument       , NULL       , FLAG_PRINT        } ,
		{ "no-confirm"    , no_argument       , NULL       , FLAG_NOCONFIRM    } ,
		{ "noconfirm"     , no_argument       , NULL       , FLAG_NOCONFIRM    } ,
		{ "no-deps"       , no_argument       , NULL       , FLAG_NODEPS       } ,
		{ "no-scriptlet"  , no_argument       , NULL       , FLAG_NOSCRIPTLET  } ,
		{ "root"          , required_argument , NULL       , FLAG_ROOT         } ,
		{ "sysupgrade"    , no_argument       , NULL       , FLAG_SYSUPGRADE   } ,
		{ "downgrade"     , no_argument       , NULL       , FLAG_DOWNGRADE    } ,

		{ "help"          , no_argument       , NULL       , FLAG_HELP         } ,
		{ "version"       , no_argument       , NULL       , FLAG_VERSION      } ,

		{ "ignore-pkg"    , required_argument , NULL       , FLAG_IGNORE_PKG   } ,
		{ "ignore"        , required_argument , NULL       , FLAG_IGNORE_PKG   } ,
		{ "ignore-group"  , required_argument , NULL       , FLAG_IGNORE_GROUP } ,
		{ "ignoregroup"   , required_argument , NULL       , FLAG_IGNORE_GROUP } ,

		{ "as-deps"       , no_argument       , NULL       , FLAG_ASDEPS       } ,
		{ "as-explicit"   , no_argument       , NULL       , FLAG_ASEXPLICIT   } ,
		{ "download-only" , no_argument       , NULL       , FLAG_DLONLY       } ,

		{ "cascade"       , no_argument       , NULL       , FLAG_CASCADE      } ,
		{ "no-backup"     , no_argument       , NULL       , FLAG_NOBACKUP     } ,
		{ "recursive"     , no_argument       , NULL       , FLAG_RECURSIVE    } ,
		{ "unneeded"      , no_argument       , NULL       , FLAG_UNNEEDED     } ,

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

			/* non-option arguments */
			case 1:
				*list = alpm_list_add(*list, strdup(optarg));
				break;

			/* action selectors */
			case FLAG_ADD:
				list = &add;
				break;
			case FLAG_FILE:
				list = &files;
				break;
			case FLAG_REMOVE:
				list = &rem;
				break;
			case FLAG_SPEC:
				list = &spec;
				break;

			/* general options */
			case FLAG_CACHEDIR:
				FREELIST(config->cachedirs);
				config->cachedirs = alpm_list_add(NULL, strdup(optarg));
				break;
			case FLAG_CONFIG:
				/* already handled */
				break;
			case FLAG_DBONLY:
				trans_flags |= ALPM_TRANS_FLAG_DBONLY;
				trans_flags |= ALPM_TRANS_FLAG_NOSCRIPTLET;
				break;
			case FLAG_DBPATH:
				free(config->dbpath);
				config->dbpath = strdup(optarg);
				break;
			case FLAG_DEBUG:
				log_level |= ALPM_LOG_DEBUG;
				log_level |= ALPM_LOG_FUNCTION;
				break;
			case FLAG_LOGFILE:
				free(config->logfile);
				config->logfile = strdup(optarg);
				break;
			case FLAG_PRINT:
				printonly = 1;
				trans_flags |= ALPM_TRANS_FLAG_NOLOCK;
				break;
			case FLAG_NOCONFIRM:
				noconfirm = 1;
				break;
			case FLAG_NODEPS:
				if(trans_flags & ALPM_TRANS_FLAG_NODEPVERSION) {
					trans_flags |= ALPM_TRANS_FLAG_NODEPS;
				} else {
					trans_flags |= ALPM_TRANS_FLAG_NODEPVERSION;
				}
				break;
			case FLAG_NOSCRIPTLET:
				trans_flags |= ALPM_TRANS_FLAG_NOSCRIPTLET;
				break;
			case FLAG_ROOT:
				free(config->rootdir);
				config->rootdir = strdup(optarg);
				break;
			case FLAG_SYSUPGRADE:
				sysupgrade = 1;
				break;

			/* sysupgrade options */
			case FLAG_DOWNGRADE:
				downgrade = 1;
				break;
			case FLAG_IGNORE_PKG:
				ignore_pkg = alpm_list_add(ignore_pkg, strdup(optarg));
				break;
			case FLAG_IGNORE_GROUP:
				ignore_group = alpm_list_add(ignore_group, strdup(optarg));
				break;

			/* install options */
			case FLAG_ASDEPS:
				if(trans_flags & ALPM_TRANS_FLAG_ALLEXPLICIT) {
					fatal("error: --asdeps and --asexplicit may not be used together\n");
				}
				trans_flags |= ALPM_TRANS_FLAG_ALLDEPS;
				break;
			case FLAG_ASEXPLICIT:
				if(trans_flags & ALPM_TRANS_FLAG_ALLDEPS) {
					fatal("error: --asdeps and --asexplicit may not be used together\n");
				}
				trans_flags |= ALPM_TRANS_FLAG_ALLEXPLICIT;
				break;
			case FLAG_DLONLY:
				trans_flags |= ALPM_TRANS_FLAG_DOWNLOADONLY;
				trans_flags |= ALPM_TRANS_FLAG_NOCONFLICTS;
				break;

			/* remove options */
			case FLAG_CASCADE:
				trans_flags |= ALPM_TRANS_FLAG_CASCADE;
				break;
			case FLAG_NOBACKUP:
				trans_flags |= ALPM_TRANS_FLAG_NOSAVE;
				break;
			case FLAG_RECURSIVE:
				if(trans_flags & ALPM_TRANS_FLAG_RECURSE) {
					trans_flags |= ALPM_TRANS_FLAG_RECURSEALL;
				} else {
					trans_flags |= ALPM_TRANS_FLAG_RECURSE;
				}
				break;
			case FLAG_UNNEEDED:
				trans_flags |= ALPM_TRANS_FLAG_UNNEEDED;
				break;

			case '?':
			default:
				usage(1);
				break;
		}
	}

	return config;
}

void cb_log(alpm_loglevel_t level, const char *fmt, va_list args)
{
	if(level & log_level) {
		vprintf(fmt, args);
	}
}

alpm_pkg_t *find_local_pkg(char *pkgspec)
{
	if(strchr(pkgspec, '/')) {
		return NULL;
	}

	return alpm_db_get_pkg(alpm_get_localdb(handle), pkgspec);
}

alpm_pkg_t *find_sync_pkg(char *pkgspec)
{
	const char *pkgname = pkgspec, *dbname = NULL;
	char *sep = strchr(pkgspec, '/');
	alpm_list_t *d;

	if(sep) {
		dbname = pkgspec;
		pkgname = sep + 1;
		*sep = '\0';
	}

	for(d = alpm_get_syncdbs(handle); d; d = d->next) {
		if(dbname && strcmp(alpm_db_get_name(d->data), dbname) != 0) {
			continue;
		}

		alpm_pkg_t *pkg = alpm_db_get_pkg(d->data, pkgname);
		if(pkg) {
			return pkg;
		}
	}
	return NULL;
}

int load_pkg_files()
{
	alpm_list_t *i;
	int ret = 0;
	alpm_siglevel_t slr = alpm_option_get_remote_file_siglevel(handle);
	alpm_siglevel_t sll = alpm_option_get_local_file_siglevel(handle);

	for(i = files; i; i = i->next) {
		alpm_siglevel_t sl = sll;
		alpm_pkg_t *pkg;

		if(strstr(i->data, "://")) {
			char *path = alpm_fetch_pkgurl(handle, i->data);
			if(!path) {
				fprintf(stderr, "error: could not load '%s' (%s)\n",
						(char*) i->data, alpm_strerror(alpm_errno(handle)));
				ret++;
				continue;
			} else {
				free(i->data);
				i->data = path;
				sl = slr;
			}
		}

		if( alpm_pkg_load(handle, i->data, 1, sl, &pkg) != 0) {
			fprintf(stderr, "error: could not load '%s' (%s)\n",
					(char*) i->data, alpm_strerror(alpm_errno(handle)));
			ret++;
		} else {
			add = alpm_list_add(add, pkg);
		}
	}

	return ret;
}

void free_fileconflict(alpm_fileconflict_t *conflict) {
	free(conflict->file);
	free(conflict->target);
	free(conflict->ctarget);
	free(conflict);
}

void print_fileconflict(alpm_fileconflict_t *conflict) {
	switch(conflict->type) {
		case ALPM_FILECONFLICT_TARGET:
			fprintf(stderr, "file conflict: '%s' exists in '%s' and '%s'\n",
				conflict->file, conflict->target, conflict->ctarget);
			break;
		case ALPM_FILECONFLICT_FILESYSTEM:
			if(conflict->ctarget && *conflict->ctarget) {
				fprintf(stderr, "file conflict: '%s' exists in '%s' and filesystem (%s)\n",
						conflict->file, conflict->target, conflict->ctarget);
			} else {
				fprintf(stderr, "file conflict: '%s' exists in '%s' and filesystem\n",
						conflict->file, conflict->target);
			}
			break;
		default:
			fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
			break;
	}
}

int main(int argc, char **argv)
{
	alpm_list_t *i, *sync_dbs = NULL, *err_data = NULL;
	int ret = 0;

	myname = pu_basename(argv[0]);
	if(strcasecmp(myname, "pacinstall") == 0) {
		list = &add;
	} else if(strcasecmp(myname, "pacremove") == 0) {
		list = &rem;
	}

	if(!(config = parse_opts(argc, argv))) {
		goto cleanup;
	}

	if(!spec && !add && !rem && !files && !sysupgrade) {
		fprintf(stderr, "error: no targets provided.\n");
		ret = 1;
		goto cleanup;
	}

	if(!(handle = pu_initialize_handle_from_config(config))) {
		fprintf(stderr, "error: failed to initialize alpm.\n");
		ret = 1;
		goto cleanup;
	}

	alpm_option_set_progresscb(handle, pu_cb_progress);
	alpm_option_set_dlcb(handle, pu_cb_download);
	alpm_option_set_logcb(handle, cb_log);

	for(i = ignore_pkg; i; i = i->next) {
		alpm_option_add_ignorepkg(handle, i->data);
	}
	for(i = ignore_group; i; i = i->next) {
		alpm_option_add_ignoregroup(handle, i->data);
	}

	if(!pu_register_syncdbs(handle, config->repos)) {
		fprintf(stderr, "error: no valid sync dbs configured.\n");
		ret = 1;
		goto cleanup;
	}

	for(i = add; i; i = i->next) {
		char *pkgspec = i->data;
		alpm_pkg_t *p = find_sync_pkg(pkgspec);
		if(p) {
			i->data = p;
		} else {
			fprintf(stderr, "error: could not locate package '%s'\n", pkgspec);
			i->data = NULL;
			ret = 1;
		}
		free(pkgspec);
	}

	for(i = rem; i; i = i->next) {
		char *pkgspec = i->data;
		alpm_pkg_t *p = find_local_pkg(pkgspec);
		if(p) {
			i->data = p;
		} else {
			fprintf(stderr, "error: could not locate package '%s'\n", pkgspec);
			i->data = NULL;
			ret = 1;
		}
		free(pkgspec);
	}

	for(i = spec; i; i = i->next) {
		char *pkgspec = i->data;
		alpm_pkg_t *p = pu_find_pkgspec(handle, pkgspec);
		if(p) {
			switch(alpm_pkg_get_origin(p)) {
				case ALPM_PKG_FROM_SYNCDB:
				case ALPM_PKG_FROM_FILE:
					add = alpm_list_add(add, p);
					break;
				case ALPM_PKG_FROM_LOCALDB:
					rem = alpm_list_add(rem, p);
					break;
			}
		} else {
			fprintf(stderr, "error: could not locate package '%s'\n", pkgspec);
			ret = 1;
		}
		free(pkgspec);
	}

	ret += load_pkg_files();

	if(ret) {
		goto cleanup;
	}

	/**************************************
	 * begin transaction
	 **************************************/

	if(alpm_trans_init(handle, trans_flags) != 0) {
		fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
		ret = 1;
		goto cleanup;
	}

	for(i = add; i; i = i->next) {
		if(alpm_add_pkg(handle, i->data) != 0) {
			fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
			ret = 1;
			goto transcleanup;
		}
	}

	for(i = rem; i; i = i->next) {
		if(alpm_remove_pkg(handle, i->data) != 0) {
			fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
			ret = 1;
			goto transcleanup;
		}
	}

	if(sysupgrade && alpm_sync_sysupgrade(handle, downgrade) != 0) {
		fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
		ret = 1;
		goto transcleanup;
	}

	if(alpm_trans_prepare(handle, &err_data) != 0) {
		fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
		ret = 1;
		goto transcleanup;
	}

	pu_display_transaction(handle);

	if(printonly || (!noconfirm && !pu_confirm(1, "Proceed with transaction")) ) {
		goto transcleanup;
	}

	pu_log_command(handle, LOG_PREFIX, argc, argv);

	if(alpm_trans_commit(handle, &err_data) != 0) {
		switch(alpm_errno(handle)) {
				case ALPM_ERR_DLT_INVALID:
				case ALPM_ERR_PKG_INVALID:
				case ALPM_ERR_PKG_INVALID_CHECKSUM:
				case ALPM_ERR_PKG_INVALID_SIG:
					for(i = err_data; i; i = i->next) {
						char *path = i->data;
						fprintf(stderr, "%s is invalid or corrupted\n", path);
						free(path);
					}
					break;

				case ALPM_ERR_FILE_CONFLICTS:
					for(i = err_data; i; i = i->next) {
						print_fileconflict(i->data);
						free_fileconflict(i->data);
					}
					break;

				default:
					/* FIXME: possible memory leak */
					fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
					break;
		}

		alpm_list_free(err_data);
		err_data = NULL;
		ret = 1;
		goto transcleanup;
	}

transcleanup:
	if(alpm_trans_release(handle) != 0) {
		fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
		ret = 1;
	}

cleanup:
	alpm_list_free(err_data);
	alpm_list_free(add);
	alpm_list_free(rem);
	FREELIST(files);
	FREELIST(ignore_pkg);
	FREELIST(ignore_group);
	alpm_release(handle);
	pu_config_free(config);

	return ret;
}

/* vim: set ts=2 sw=2 noet: */
