#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <errno.h>
#include <getopt.h>
#include <time.h>

#include <pacutils.h>
#include <pacutils/log.h>

const char *myname = "paclog", *myver = "0.1";

char *logfile = NULL;

time_t after = 0, before = 0;
alpm_list_t *pkgs = NULL;
int color = 1;

enum longopt_flags {
	FLAG_CONFIG = 1000,
	FLAG_AFTER,
	FLAG_BEFORE,
	FLAG_HELP,
	FLAG_LOGFILE,
	FLAG_PACKAGE,
	FLAG_VERSION,
};

struct {
	const char *timestamp;
	const char *caller;

	const char *message;
	const char *warning;
	const char *error;
	const char *note;
	const char *action;
	const char *install;
	const char *uninstall;

	const char *reset;
} palette = {
	.timestamp = "\e[33m",       // yellow
	.caller    = "\e[34m",       // blue

	.message   = "\e[0m",        // normal
	.warning   = "\e[37;41m",    // white on red
	.error     = "\e[37;41m",    // white on red
	.note      = "\e[34m",       // blue

	.action    = "\e[36m",       // cyan
	.install   = "\e[32m",       // green
	.uninstall = "\e[31m",       // red
	.reset     = "\e[0m",
};

void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
#define hputs(s) fputs(s"\n", stream);
	hputs("paclog - filter pacman log entries");
	hputs("usage: paclog [options] [filters]");
	hputs("");
	hputs("options:");
	hputs("   --config=<path>     set an alternate configuration file");
	hputs("   --debug             enable extra debugging messages");
	hputs("   --logfile=<path>    set an alternate log file");
	hputs("   --[no]-color        color output");
	hputs("");
	hputs("filters:");
	hputs("   --package=<pkg>     show entries affecting <pkg>");
	hputs("   --before=<date>     show entries before <date>");
	hputs("   --after=<date>      show entries after <date>");
#undef hputs
	exit(ret);
}

int parse_time(char *string, time_t *dest)
{
	char *c = string;
	struct tm stm;
	memset(&stm, 0, sizeof(struct tm));

#define parse_bit(s, f, t) \
	do { \
		if(!(s = strptime(s, f, t))) { \
			return 0; \
		} \
		if(!*s) { \
			*dest = mktime(t); \
			return 1; \
		} \
	} while(0)
	parse_bit(c, "%Y", &stm);
	parse_bit(c, "-%m", &stm);
	parse_bit(c, "-%d", &stm);
	parse_bit(c, " %H:%M", &stm);
#undef parse_bit

	return 0;
}

void parse_opts(int argc, char **argv)
{
	char *config_file = "/etc/pacman.conf";
	int c;

	const char *short_opts = "";
	struct option long_opts[] = {
		{ "color",      no_argument,       &color, 2            },
		{ "no-color",   no_argument,       &color, 0            },
		{ "config",     required_argument, NULL, FLAG_CONFIG    } ,
		{ "logfile",    required_argument, NULL, FLAG_LOGFILE   } ,
		{ "help",       no_argument,       NULL, FLAG_HELP      } ,
		{ "version",    no_argument,       NULL, FLAG_VERSION   } ,

		{ "after",      required_argument, NULL, FLAG_AFTER     } ,
		{ "before",     required_argument, NULL, FLAG_BEFORE    } ,
		{ "package",    required_argument, NULL, FLAG_PACKAGE   } ,
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

			case FLAG_AFTER:
				if(!parse_time(optarg, &after)) {
					fprintf(stderr, "Unable to parse date '%s'\n", optarg);
					exit(1);
				}
				break;
			case FLAG_BEFORE:
				if(!parse_time(optarg, &before)) {
					fprintf(stderr, "Unable to parse date '%s'\n", optarg);
					exit(1);
				}
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

int fprint_entry_color(FILE *stream, pu_log_entry_t *entry)
{
	int ret = 0;
	char timestamp[50];
	char *c, *message = strdup(entry->message);
	const char *message_color;
	pu_log_action_t *a = pu_log_action_parse(entry->message);

	if(a) {
		switch(a->operation) {
			case PU_LOG_OPERATION_INSTALL:
				message_color = palette.install;
				break;
			case PU_LOG_OPERATION_REMOVE:
				message_color = palette.uninstall;
				break;
			default:
				message_color = palette.action;
				break;
		}
		pu_log_action_free(a);
	} else if(strncmp(entry->message, "warning: ", 9) == 0) {
		message_color = palette.warning;
	} else if(strncmp(entry->message, "error: ", 7) == 0) {
		message_color = palette.error;
	} else if(strncmp(entry->message, "note: ", 6) == 0) {
		message_color = palette.note;
	} else {
		message_color = palette.message;
	}

	strftime(timestamp, 50, "%F %R", entry->timestamp);

	/* strip trailing newline so colors don't span line breaks */
	if(*(c = message + strlen(message) - 1) == '\n') {
		*c = '\0';
	}

	if(entry->caller) {
		ret = printf("[%s%s%s] [%s%s%s] %s%s%s\n",
				palette.timestamp, timestamp,     palette.reset,
				palette.caller,    entry->caller, palette.reset,
				message_color,     message,       palette.reset);
	} else {
		ret = printf("[%s%s%s] %s%s%s\n",
				palette.timestamp, timestamp, palette.reset,
				message_color,     message,   palette.reset);
	}

	free(message);
	return ret;
}

void print_entry(FILE *stream, pu_log_entry_t *entry) {
	if(color) {
		fprint_entry_color(stream, entry);
	} else {
		pu_log_fprint_entry(stream, entry);
	}
}

int main(int argc, char **argv)
{
	alpm_list_t *i, *entries = NULL;
	FILE *f;
	int ret = 0;

	parse_opts(argc, argv);
	if(color == 1 && !isatty(fileno(stdout))) {
		color = 0;
	}

	if(!isatty(fileno(stdin)) && !feof(stdin)) {
		free(logfile);
		logfile = strdup("<stdin>");
		f = stdin;
	} else if(!(f = fopen(logfile, "r"))) {
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

	if(!after && !before && !pkgs) {
		for(i = entries; i; i = i->next) {
			pu_log_entry_t *e = i->data;
			print_entry(stdout, e);
		}
	} else {
		for(i = entries; i; i = i->next) {
			pu_log_entry_t *e = i->data;

			if(after) {
				if(mktime(e->timestamp) >= after) {
					print_entry(stdout, e);
					continue;
				}
			}

			if(before) {
				if(mktime(e->timestamp) <= before) {
					print_entry(stdout, e);
					continue;
				}
			}

			if(pkgs) {
				pu_log_action_t *a = pu_log_action_parse(e->message);
				int found = (a && alpm_list_find_str(pkgs, a->target));
				pu_log_action_free(a);
				if(found) {
					print_entry(stdout, e);
					continue;
				}
			}
		}
	}

cleanup:
	FREELIST(pkgs);
	alpm_list_free_inner(entries, (alpm_list_fn_free) pu_log_entry_free);
	alpm_list_free(entries);
	return ret;
}

/* vim: set ts=2 sw=2 noet: */
