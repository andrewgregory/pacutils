#include <getopt.h>

#include <pacutils.h>

#define LOG_PREFIX "PACSYNC"

pu_config_t *config = NULL;
alpm_handle_t *handle = NULL;
alpm_loglevel_t log_level = ALPM_LOG_ERROR | ALPM_LOG_WARNING;
alpm_transflag_t trans_flags = 0;
int force = 0;

enum longopt_flags {
	FLAG_CONFIG = 1000,
	FLAG_DBPATH,
	FLAG_DEBUG,
	FLAG_HELP,
	FLAG_LOGFILE,
	FLAG_VERSION,
};

#define hputs(msg) fputs(msg"\n", stream);
void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
	hputs("pacsync - synchronize sync databases and packages");
	hputs("usage:  pacsync [options] [repo]...");
	hputs("        pacsync (--help|--version)");
	hputs("options:");
	hputs("   --config=<path>    set an alternate configuration file");
	hputs("   --dbpath=<path>    set an alternate database location");
	hputs("   --force            sync repos even if already up-to-date");
	hputs("   --debug            enable extra debugging messages");
	hputs("   --logfile=<path>   set an alternate log file");
	hputs("   --help             display this help information");
	hputs("   --version          display version information");
	exit(ret);
}
#undef hputs

void version(void)
{
	printf("pacsync v0.1 - libalpm %s\n", alpm_version());
	exit(0);
}

pu_config_t *parse_opts(int argc, char **argv)
{
	char *config_file = "/etc/pacman.conf";
	int c;

	char *short_opts = "";
	struct option long_opts[] = {
		{ "config"       , required_argument , NULL         , FLAG_CONFIG   } ,
		{ "dbpath"       , required_argument , NULL         , FLAG_DBPATH   } ,
		{ "debug"        , no_argument       , NULL         , FLAG_DEBUG    } ,
		{ "force"        , no_argument       , &force       , 1             } ,
		{ "help"         , no_argument       , NULL         , FLAG_HELP     } ,
		{ "version"      , no_argument       , NULL         , FLAG_VERSION  } ,
		{ "logfile"      , required_argument , NULL         , FLAG_LOGFILE  } ,
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
			case FLAG_DBPATH:
				free(config->dbpath);
				config->dbpath = strdup(optarg);
				break;
			case FLAG_DEBUG:
				log_level |= ALPM_LOG_DEBUG;
				log_level |= ALPM_LOG_FUNCTION;
				break;
			case FLAG_HELP:
				usage(0);
				break;
			case FLAG_LOGFILE:
				free(config->logfile);
				config->logfile = strdup(optarg);
				break;
			case FLAG_VERSION:
				version();
				break;
			case '?':
				usage(1);
				break;
		}
	}

	if(!pu_ui_config_load(config, config_file)) {
		fprintf(stderr, "error: could not parse '%s'\n", config_file);
		return NULL;
	}

	return config;
}

void cb_log(alpm_loglevel_t level, const char *fmt, va_list args)
{
	if(level & log_level) {
		vprintf(fmt, args);
	}
}

int main(int argc, char **argv)
{
	alpm_list_t *i, *sync_dbs = NULL, *targets = NULL;
	int ret = 0;

	if(!parse_opts(argc, argv)) {
		goto cleanup;
	}

	if(!(handle = pu_initialize_handle_from_config(config))) {
		fprintf(stderr, "error: failed to initialize alpm.\n");
		ret = 1;
		goto cleanup;
	}

	alpm_option_set_progresscb(handle, pu_ui_cb_progress);
	alpm_option_set_dlcb(handle, pu_ui_cb_download);
	alpm_option_set_logcb(handle, cb_log);

	sync_dbs = pu_register_syncdbs(handle, config->repos);
	if(!sync_dbs) {
		fprintf(stderr, "error: no valid sync dbs configured.\n");
		ret = 1;
		goto cleanup;
	}

	if(optind >= argc) {
		targets = sync_dbs;
	} else {
		for(; optind < argc; optind++) {
			int found = 0;
			for(i = sync_dbs; i; i = i->next) {
				if(strcmp(argv[optind], alpm_db_get_name(i->data)) == 0) {
					targets = alpm_list_add(targets, i->data);
					found = 1;
					break;
				}
			}
			if(!found) {
				ret = 1;
				fprintf(stderr, "error: no sync db '%s' configured\n", argv[optind]);
			}
		}
	}

	if(ret != 0) {
		goto cleanup;
	}

	pu_log_command(handle, LOG_PREFIX, argc, argv);

	for(i = targets; i; i = i->next) {
		alpm_db_t *db = i->data;
		int res = alpm_db_update(force, db);
		if(res < 0) {
			ret = 1;
			fprintf(stderr, "error: could not sync db '%s' (%s)\n",
					alpm_db_get_name(db), alpm_strerror(alpm_errno(handle)));
		} else if(res == 1) {
			/* db was already up to date */
			printf("%s is up to date\n", alpm_db_get_name(db));
		}
		/* else: callbacks display relevant information */
	}

cleanup:
	if(targets != sync_dbs) {
		alpm_list_free(targets);
	}
	alpm_release(handle);
	pu_config_free(config);

	return ret;
}

/* vim: set ts=2 sw=2 noet: */
