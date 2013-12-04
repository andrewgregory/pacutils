#include <errno.h>
#include <getopt.h>

#include <pacutils.h>
#include <pacutils/log.h>

const char *myname = "paclog", *myver = "0.1";

char *logfile = NULL;

alpm_list_t *pkgs = NULL;

enum longopt_flags {
	FLAG_CONFIG = 1000,
	FLAG_HELP,
	FLAG_LOGFILE,
	FLAG_PACKAGE,
	FLAG_VERSION,
};

void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
#define hputs(s) fputs(s"\n", stream);
	hputs("paclog - filter pacman log entries");
	hputs("usage: paclog [options] [filters]");
	hputs("");
	hputs("options:");
	hputs("   --config=<path>	 set an alternate configuration file");
	hputs("   --debug			 enable extra debugging messages");
	hputs("   --logfile=<path>	 set an alternate log file");
	hputs("");
	hputs("filters:");
	hputs("   --package=<pkg>	 show entries affecting <pkg>");
#undef hputs
	exit(ret);
}

void parse_opts(int argc, char **argv)
{
	char *config_file = "/etc/pacman.conf";
	int c;

	const char *short_opts = "";
	struct option long_opts[] = {
		{ "config"		, required_argument , NULL , FLAG_CONFIG	   } ,
		{ "logfile"		, required_argument , NULL , FLAG_LOGFILE	   } ,
		{ "help"		, no_argument		, NULL , FLAG_HELP		   } ,
		{ "version"		, no_argument		, NULL , FLAG_VERSION	   } ,

		{ "package"		, required_argument , NULL , FLAG_PACKAGE	   } ,
	};

	while((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch(c) {
			case FLAG_CONFIG:
				config_file = optarg;
				break;
			case FLAG_HELP:
				usage(0);
				break;
			case FLAG_LOGFILE:
				logfile = strdup(optarg);
				break;
			case FLAG_VERSION:
				pu_print_version(myname, myver);
				exit(0);
				break;

			case FLAG_PACKAGE:
				pkgs = alpm_list_add(pkgs, strdup(optarg));
				break;
		}
	}

	if(!logfile) {
		pu_config_t *config = pu_config_new_from_file(config_file);
		if(config) {
			logfile = strdup(config->logfile);
			pu_config_free(config);
		} else {
			logfile = strdup("/var/log/pacman.log");
		}
	}
}

int main(int argc, char **argv)
{
	alpm_list_t *i, *entries = NULL;
	FILE *f;
	int ret = 0;

	parse_opts(argc, argv);

	if(!(f = fopen(logfile, "r"))) {
		fprintf(stderr, "error: could not open '%s' for reading (%s)\n",
				logfile, strerror(errno));
		ret = 1;
		goto cleanup;
	}

	entries = pu_log_parse_file(f);
	fclose(f);

	if(!entries) {
		fprintf(stderr, "error: could not parse '%s'\n", logfile);
		ret = 1;
		goto cleanup;
	}

	for(i = entries; i; i = i->next) {
		pu_log_entry_t *e = i->data;

		if(pkgs) {
			pu_log_action_t *a = pu_log_action_parse(e->message);
			int found = (a && alpm_list_find_str(pkgs, a->target));
			pu_log_action_free(a);
			if(!found) continue;
		}

		pu_log_fprint_entry(stdout, e);
	}

cleanup:
	FREELIST(pkgs);
	alpm_list_free_inner(entries, (alpm_list_fn_free) pu_log_entry_free);
	alpm_list_free(entries);
	return ret;
}

/* vim: set ts=2 sw=2 noet: */
