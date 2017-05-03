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

#include <getopt.h>

#include <pacutils.h>

#include "config-defaults.h"

const char *myname = "pacsync", *myver = BUILDVER;
#define LOG_PREFIX "PACSYNC"

pu_config_t *config = NULL;
alpm_handle_t *handle = NULL;
alpm_loglevel_t log_level = ALPM_LOG_ERROR | ALPM_LOG_WARNING;
alpm_transflag_t trans_flags = 0;
int force = 0, ret_updated = 0;
const char *dbext = NULL, *sysroot = NULL;

enum longopt_flags {
	FLAG_CONFIG = 1000,
	FLAG_DBEXT,
	FLAG_DBPATH,
	FLAG_DEBUG,
	FLAG_HELP,
	FLAG_LOGFILE,
	FLAG_SYSROOT,
	FLAG_VERSION,
};

#define hputs(msg) fputs(msg"\n", stream);
void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
	hputs("pacsync - synchronize sync databases");
	hputs("usage:  pacsync [options] [repo]...");
	hputs("        pacsync (--help|--version)");
	hputs("options:");
	hputs("   --config=<path>    set an alternate configuration file");
	hputs("   --dbext=<ext>      set an alternate sync database extension");
	hputs("   --dbpath=<path>    set an alternate database location");
	hputs("   --sysroot=<path>   set an alternate system root");
	hputs("   --force            sync repos even if already up-to-date");
	hputs("   --debug            enable extra debugging messages");
	hputs("   --logfile=<path>   set an alternate log file");
	hputs("   --updated          return false unless a database was updated");
	hputs("   --help             display this help information");
	hputs("   --version          display version information");
	exit(ret);
}
#undef hputs

pu_config_t *parse_opts(int argc, char **argv)
{
	char *config_file = PACMANCONF;
	int c;

	char *short_opts = "";
	struct option long_opts[] = {
		{ "config"       , required_argument , NULL         , FLAG_CONFIG   } ,
		{ "dbext"        , required_argument , NULL         , FLAG_DBEXT    } ,
		{ "dbpath"       , required_argument , NULL         , FLAG_DBPATH   } ,
		{ "debug"        , no_argument       , NULL         , FLAG_DEBUG    } ,
		{ "sysroot"      , required_argument , NULL         , FLAG_SYSROOT  } ,
		{ "force"        , no_argument       , &force       , 1             } ,
		{ "updated"      , no_argument       , &ret_updated , 1             } ,
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
			case FLAG_DBEXT:
				dbext = optarg;
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
			case FLAG_SYSROOT:
				sysroot = optarg;
				break;
			case FLAG_VERSION:
				pu_print_version(myname, myver);
				break;
			case '?':
				usage(1);
				break;
		}
	}

	if(!pu_ui_config_load_sysroot(config, config_file, sysroot)) {
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
	int ret = 0, updated = 0;

	if(!parse_opts(argc, argv)) {
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
		} else {
			/* callbacks display relevant information */
			updated = 1;
		}
	}

cleanup:
	if(targets != sync_dbs) {
		alpm_list_free(targets);
	}
	alpm_release(handle);
	pu_config_free(config);

	if(ret == 0 && ret_updated && updated == 0) {
		ret = 1;
	}

	return ret;
}

/* vim: set ts=2 sw=2 noet: */
