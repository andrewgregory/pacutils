#include <getopt.h>

#include <pacutils.h>

#define MYNAME "pactrans"
#define VERSION "0.1"
#define LOG_PREFIX "PACTRANS"

pu_config_t *config = NULL;
alpm_handle_t *handle = NULL;

alpm_loglevel_t log_level = ALPM_LOG_ERROR | ALPM_LOG_WARNING;
alpm_transflag_t trans_flags = 0;

alpm_list_t *spec = NULL, *add = NULL, *rem = NULL, *files = NULL;
alpm_list_t **list = &spec;
int printonly = 0;

enum longopt_flags {
	FLAG_ADD = 1000,
	FLAG_ASDEPS,
	FLAG_ASEXPLICIT,
	FLAG_CACHEDIR,
	FLAG_CONFIG,
	FLAG_DBPATH,
	FLAG_DEBUG,
	FLAG_DLONLY,
	FLAG_FILE,
	FLAG_HELP,
	FLAG_LOGFILE,
	FLAG_RECURSIVE,
	FLAG_REMOVE,
	FLAG_ROOT,
	FLAG_SPEC,
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
#define hputs(s) fputs(s"\n", stream);
	hputs("pactrans - install and remove packages in a single transaction");
	hputs("usage:  pactrans [options] ([<action>] <target>...)...");
	hputs("        pactrans (--help|--version)");
	hputs("");
	hputs("actions (may be used together):");
	hputs("   --spec             install sync/file specs, remove local specs (default)");
	hputs("   --add              install packages from sync database");
	hputs("   --file             install packages from files");
	hputs("   --remove           remove packages");
	hputs("");
	hputs("options:");
	hputs("   --nodeps");
	hputs("   --dbonly");
	hputs("   --noscriptlet");
	hputs("   --print-only");
	hputs("   --cachedir=<path>  set an alternate cache location");
	hputs("   --config=<path>    set an alternate configuration file");
	hputs("   --dbpath=<path>    set an alternate database location");
	hputs("   --debug            enable extra debugging messages");
	hputs("   --logfile=<path>   set an alternate log file");
	hputs("   --root=<path>      set an alternate installation root");
	hputs("   --help             display this help information");
	hputs("   --version          display version information");
	hputs("");
	hputs("add/file options:");
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
		{ "add"           , no_argument       , NULL       , FLAG_ADD          } ,
		{ "file"          , no_argument       , NULL       , FLAG_FILE         } ,
		{ "remove"        , no_argument       , NULL       , FLAG_REMOVE       } ,
		{ "asdeps"        , no_argument       , NULL       , FLAG_ASDEPS       } ,
		{ "asexplicit"    , no_argument       , NULL       , FLAG_ASEXPLICIT   } ,
		{ "recursive"     , no_argument       , NULL       , FLAG_RECURSIVE    } ,
		{ "config"        , required_argument , NULL       , FLAG_CONFIG       } ,
		{ "dbpath"        , required_argument , NULL       , FLAG_DBPATH       } ,
		{ "debug"         , no_argument       , NULL       , FLAG_DEBUG        } ,
		{ "download-only" , no_argument       , NULL       , FLAG_DLONLY       } ,
		{ "help"          , no_argument       , NULL       , FLAG_HELP         } ,
		{ "root"          , required_argument , NULL       , FLAG_ROOT         } ,
		{ "version"       , no_argument       , NULL       , FLAG_VERSION      } ,
		{ "logfile"       , required_argument , NULL       , FLAG_LOGFILE      } ,
		{ "cachedir"      , required_argument , NULL       , FLAG_CACHEDIR     } ,
		{ "print-only"    , no_argument       , &printonly , 1                 } ,
		{ 0, 0, 0, 0 },
	};

	/* check for a custom config file location */
	while((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		if(c == FLAG_CONFIG) {
			config_file = optarg;
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

			/* already handled */
			case 0:
			case FLAG_CONFIG:
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

			/* options */
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
			case FLAG_RECURSIVE:
				if(trans_flags & ALPM_TRANS_FLAG_RECURSE) {
					trans_flags |= ALPM_TRANS_FLAG_RECURSEALL;
				} else {
					trans_flags |= ALPM_TRANS_FLAG_RECURSE;
				}
				break;
			case FLAG_CACHEDIR:
				FREELIST(config->cachedirs);
				config->cachedirs = alpm_list_add(NULL, strdup(optarg));
				break;
			case FLAG_DBPATH:
				free(config->dbpath);
				config->dbpath = strdup(optarg);
				break;
			case FLAG_DEBUG:
				log_level |= ALPM_LOG_DEBUG;
				log_level |= ALPM_LOG_FUNCTION;
				break;
			case FLAG_DLONLY:
				trans_flags |= ALPM_TRANS_FLAG_DOWNLOADONLY;
				trans_flags |= ALPM_TRANS_FLAG_NOCONFLICTS;
				break;
			case FLAG_HELP:
				usage(0);
				break;
			case FLAG_LOGFILE:
				free(config->logfile);
				config->logfile = strdup(optarg);
				break;
			case FLAG_ROOT:
				free(config->rootdir);
				config->rootdir = strdup(optarg);
				break;
			case FLAG_VERSION:
				pu_print_version(MYNAME, VERSION);
				exit(0);
				break;
			case '?':
			default:
				usage(1);
				break;
		}
	}

	return config;
}

void cb_event(alpm_event_t event, void *data1, void *data2)
{
	switch(event) {
		case ALPM_EVENT_ADD_DONE:
			alpm_logaction(handle, LOG_PREFIX, "installed %s (%s)\n",
					alpm_pkg_get_name(data1), alpm_pkg_get_version(data1));
			break;
		case ALPM_EVENT_REMOVE_DONE:
			alpm_logaction(handle, LOG_PREFIX, "removed %s (%s)\n",
					alpm_pkg_get_name(data1), alpm_pkg_get_version(data1));
			break;
		case ALPM_EVENT_UPGRADE_DONE:
			alpm_logaction(handle, LOG_PREFIX, "upgraded %s (%s -> %s)\n",
					alpm_pkg_get_name(data1),
					alpm_pkg_get_version(data2), alpm_pkg_get_version(data1));
			break;
		case ALPM_EVENT_DOWNGRADE_DONE:
			alpm_logaction(handle, LOG_PREFIX, "downgraded %s (%s -> %s)\n",
					alpm_pkg_get_name(data1),
					alpm_pkg_get_version(data2), alpm_pkg_get_version(data1));
			break;
		case ALPM_EVENT_REINSTALL_DONE:
			alpm_logaction(handle, LOG_PREFIX, "reinstalled %s (%s)\n",
					alpm_pkg_get_name(data1), alpm_pkg_get_version(data1));
			break;
	}
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

int main(int argc, char **argv)
{
	alpm_list_t *i, *sync_dbs = NULL, *err_data = NULL;
	int ret = 0;

	if(!(config = parse_opts(argc, argv))) {
		goto cleanup;
	}

	if(!spec && !add && !rem && !files) {
		fprintf(stderr, "error: no targets provided.\n");
		ret = 1;
		goto cleanup;
	}

	if(!(handle = pu_initialize_handle_from_config(config))) {
		fprintf(stderr, "error: failed to initialize alpm.\n");
		ret = 1;
		goto cleanup;
	}

	alpm_option_set_eventcb(handle, cb_event);
	alpm_option_set_progresscb(handle, pu_cb_progress);
	alpm_option_set_dlcb(handle, pu_cb_download);
	alpm_option_set_logcb(handle, cb_log);

	sync_dbs = pu_register_syncdbs(handle, config->repos);
	if(!sync_dbs) {
		fprintf(stderr, "error: no valid sync dbs configured.\n");
		ret = 1;
		goto cleanup;
	}

	for(i = sync_dbs; i; i = i->next) {
		alpm_db_set_usage(i->data, ALPM_DB_USAGE_ALL);
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

	if(alpm_trans_init(handle, trans_flags) != 0) {
		fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
		ret = 1;
		goto cleanup;
	}

	for(i = add; i; i = i->next) {
		alpm_add_pkg(handle, i->data);
	}

	for(i = rem; i; i = i->next) {
		alpm_remove_pkg(handle, i->data);
	}

	if(alpm_trans_prepare(handle, &err_data) != 0) {
		fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
		ret = 1;
		goto transcleanup;
	}

	pu_display_transaction(handle);

	if(printonly || !pu_confirm(1, "Proceed with transaction") ) {
		goto transcleanup;
	}

	if(alpm_trans_commit(handle, &err_data) != 0) {
		fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
		ret = 1;
		goto transcleanup;
	}

transcleanup:
	if(alpm_trans_release(handle) != 0) {
		fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
		ret = 1;
	}

cleanup:
	alpm_list_free(sync_dbs);
	alpm_list_free(err_data);
	alpm_list_free(add);
	alpm_list_free(rem);
	FREELIST(files);
	alpm_release(handle);
	pu_config_free(config);

	return ret;
}

/* vim: set ts=2 sw=2 noet: */
