#include <getopt.h>

#include <pacutils.h>

#define LOG_PREFIX "PACSYNC"

pu_config_t *config = NULL;
alpm_handle_t *handle = NULL;
alpm_loglevel_t log_level = ALPM_LOG_ERROR | ALPM_LOG_WARNING;
int force = 0;

enum longopt_flags {
	FLAG_CONFIG,
	FLAG_DBPATH,
	FLAG_DEBUG,
	FLAG_FORCE,
	FLAG_HELP,
	FLAG_LOGFILE,
	FLAG_VERSION,
};

void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
	fputs("pacsync - synchronize sync databases\n", stream);
	fputs("usage:  pacsync [options] [repo]...\n", stream);
	fputs("        pacsync (--help|--version)\n", stream);
	fputs("options:\n", stream);
	fputs("   --config=<path>    set an alternate configuration file\n", stream);
	fputs("   --dbpath=<path>    set an alternate database location\n", stream);
	fputs("   --force            sync repos even if already up-to-date\n", stream);
	fputs("   --debug            enable extra debugging messages\n", stream);
	fputs("   --logfile=<path>   set an alternate log file\n", stream);
	fputs("   --help             display this help information\n", stream);
	fputs("   --version          display version information\n", stream);
	exit(ret);
}

void version(void)
{
	printf("pacsync v0.1 - libalpm %s\n", alpm_version());
	exit(0);
}

pu_config_t *parse_opts(int argc, char **argv)
{
	char *config_file = "/etc/pacman.conf";
	pu_config_t *config = NULL;
	int c;

	char *short_opts = "";
	struct option long_opts[] = {
		{ "config"   , required_argument , NULL , FLAG_CONFIG   } ,
		{ "dbpath"   , required_argument , NULL , FLAG_DBPATH   } ,
		{ "debug"    , no_argument       , NULL , FLAG_DEBUG    } ,
		{ "force"    , no_argument       , NULL , FLAG_FORCE    } ,
		{ "help"     , no_argument       , NULL , FLAG_HELP     } ,
		{ "version"  , no_argument       , NULL , FLAG_VERSION  } ,
		{ "logfile"  , required_argument , NULL , FLAG_LOGFILE  } ,
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
			case FLAG_DEBUG:
				log_level |= ALPM_LOG_DEBUG;
				log_level |= ALPM_LOG_FUNCTION;
				break;
			case FLAG_FORCE:
				force = 1;
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
			default:
				usage(1);
				break;
		}
		c = getopt_long(argc, argv, short_opts, long_opts, NULL);
	}

	return config;
}

void cb_download(const char *filename, off_t xfered, off_t total)
{
	printf("downloading %s (%d/%d)\n", filename, xfered, total);
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

	if(!(config = parse_opts(argc, argv))) {
		goto cleanup;
	}

	if(!(handle = pu_initialize_handle_from_config(config))) {
		fprintf(stderr, "error: failed to initialize alpm.\n");
		ret = 1;
		goto cleanup;
	}

	alpm_option_set_dlcb(handle, cb_download);
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
					found = 1;
					targets = alpm_list_add(targets, i->data);
				}
			}
			if(!found) {
				ret = 1;
				fprintf(stderr, "error: no sync db '%s' configured\n");
			}
		}
	}

	if(ret != 0) {
		goto cleanup;
	}

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
	alpm_list_free(sync_dbs);
	alpm_release(handle);
	pu_config_free(config);

	return ret;
}

/* vim: set ts=2 sw=2 noet: */
