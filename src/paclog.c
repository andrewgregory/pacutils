#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <errno.h>
#include <getopt.h>
#include <regex.h>
#include <strings.h>
#include <time.h>

#include <pacutils.h>
#include <pacutils/log.h>

const char *myname = "paclog", *myver = "0.2";

char *logfile = NULL;

time_t after = 0, before = 0;
alpm_list_t *pkgs = NULL, *caller = NULL, *actions = NULL, *grep = NULL;
int color = 1, warnings = 0, list_installed = 0, commandline = 0;

enum longopt_flags {
	FLAG_CONFIG = 1000,
	FLAG_ACTION,
	FLAG_AFTER,
	FLAG_BEFORE,
	FLAG_CALLER,
	FLAG_COMMAND,
	FLAG_GREP,
	FLAG_HELP,
	FLAG_INSTALLED,
	FLAG_LOGFILE,
	FLAG_PACKAGE,
	FLAG_ROOT,
	FLAG_VERSION,
	FLAG_WARNINGS,
};

#define ESC "\x1B" /* "\e" */
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
	const char *transaction;

	const char *reset;
} palette = {
	.timestamp = ESC"[33m",       // yellow
	.caller    = ESC"[34m",       // blue

	.message   = ESC"[0m",        // normal
	.warning   = ESC"[37;41m",    // white on red
	.error     = ESC"[37;41m",    // white on red
	.note      = ESC"[34m",       // blue

	.transaction = ESC"[35m",

	.action    = ESC"[36m",       // cyan
	.install   = ESC"[32m",       // green
	.uninstall = ESC"[31m",       // red
	.reset     = ESC"[0m",
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
	hputs("   --root=<path>       set an alternate installation root");
	hputs("   --debug             enable extra debugging messages");
	hputs("   --logfile=<path>    set an alternate log file");
	hputs("   --[no-]color        color output");
	hputs("   --pkglist           list installed packages (EXPERIMENTAL)");
	hputs("");
	hputs("filters:");
	hputs("   --action=<action>   show <action> entries");
	hputs("   --after=<date>      show entries after <date>");
	hputs("   --before=<date>     show entries before <date>");
	hputs("   --caller=<name>     show entries from program <name>");
	hputs("   --commandline       show command line entries");
	hputs("   --grep=<regex>      show entries matching <regex>");
	hputs("   --package=<pkg>     show entries affecting <pkg>");
	hputs("   --warnings          show notes/warnings/errors");
#undef hputs
	exit(ret);
}

int parse_time(char *string, time_t *dest)
{
	char *c;
	struct tm stm;
	memset(&stm, 0, sizeof(struct tm));
	stm.tm_isdst = -1;

	if((c = strchr(string, 'T'))) {
		/* ISO 8601 format */
		char *sep = c;
		*sep = ' ';
		if((c = strchr(sep, ':')) && (c = strchr(c + 1, ':'))) {
			/* seconds */
			*c = '\0';
		} else if((c = strchr(sep, '-'))) {
			/* time zone */
			*c = '\0';
		}
	}

	c = string;
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
		{ "color",      no_argument,       &color, 2            } ,
		{ "no-color",   no_argument,       &color, 0            } ,
		{ "config",     required_argument, NULL, FLAG_CONFIG    } ,
		{ "root",       required_argument, NULL, FLAG_ROOT      } ,
		{ "logfile",    required_argument, NULL, FLAG_LOGFILE   } ,
		{ "help",       no_argument,       NULL, FLAG_HELP      } ,
		{ "version",    no_argument,       NULL, FLAG_VERSION   } ,

		{ "pkglist",    no_argument,       NULL, FLAG_INSTALLED } ,

		{ "action",     required_argument, NULL, FLAG_ACTION    } ,
		{ "after",      required_argument, NULL, FLAG_AFTER     } ,
		{ "before",     required_argument, NULL, FLAG_BEFORE    } ,
		{ "caller",     required_argument, NULL, FLAG_CALLER    } ,
		{ "commandline",no_argument,       NULL, FLAG_COMMAND   } ,
		{ "grep",       required_argument, NULL, FLAG_GREP      } ,
		{ "package",    required_argument, NULL, FLAG_PACKAGE   } ,
		{ "warnings",   no_argument,       NULL, FLAG_WARNINGS  } ,

		{ 0, 0, 0, 0 },
	};

	while((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch(c) {
			case 1:
				break;

			case FLAG_CONFIG:
				config_file = optarg;
				break;
			case FLAG_ROOT:
				free(logfile);
				logfile = malloc(strlen(optarg) + strlen("/var/log/pacman.log") + 1);
				sprintf(logfile, "%s/var/log/pacman.log", optarg);
				break;
			case FLAG_HELP:
				usage(0);
				break;
			case FLAG_LOGFILE:
				free(logfile);
				logfile = strdup(optarg);
				break;
			case FLAG_VERSION:
				pu_print_version(myname, myver);
				exit(0);
				break;

			case FLAG_INSTALLED:
				list_installed = 1;
				break;

			case FLAG_ACTION:
				actions = alpm_list_add(actions, strdup(optarg));
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
			case FLAG_CALLER:
				caller = alpm_list_add(caller, strdup(optarg));
				break;
			case FLAG_COMMAND:
				commandline = 1;
				break;
			case FLAG_GREP:
				{
					regex_t *preg = calloc(sizeof(regex_t), 1);
					if(regcomp(preg, optarg, REG_EXTENDED | REG_NOSUB | REG_ICASE) != 0) {
						fprintf(stderr, "Unable to compile regex '%s'\n", optarg);
						exit(1);
					}
					grep = alpm_list_add(grep, preg);
				}
				break;
			case FLAG_PACKAGE:
				pkgs = alpm_list_add(pkgs, strdup(optarg));
				break;
			case FLAG_WARNINGS:
				warnings = 1;
				break;

			case '?':
				usage(2);
				break;
		}
	}

	if(!logfile) {
		pu_config_t *config = pu_ui_config_load(NULL, config_file);
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
	} else if(strcmp(entry->message, "transaction started\n") == 0) {
		message_color = palette.transaction;
	} else if(strcmp(entry->message, "transaction completed\n") == 0) {
		message_color = palette.transaction;
	} else if(strncmp(entry->message, "transaction ", 12) == 0) {
		message_color = palette.error;
	} else {
		message_color = palette.message;
	}

	strftime(timestamp, 50, "%F %R", &entry->timestamp);

	/* strip trailing newline so colors don't span line breaks */
	if(*(c = message + strlen(message) - 1) == '\n') {
		*c = '\0';
	}

	if(entry->caller) {
		ret = fprintf(stream, "[%s%s%s] [%s%s%s] %s%s%s\n",
				palette.timestamp, timestamp,     palette.reset,
				palette.caller,    entry->caller, palette.reset,
				message_color,     message,       palette.reset);
	} else {
		ret = fprintf(stream, "[%s%s%s] %s%s%s\n",
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

	if(!isatty(fileno(stdin)) && errno != EBADF) {
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

	if(list_installed) {
		alpm_list_t *seen = NULL;

		for(i = alpm_list_last(entries); i; i = alpm_list_previous(i)) {
			pu_log_entry_t *e = i->data;
			pu_log_action_t *a = pu_log_action_parse(e->message);

			if(a && !alpm_list_find_str(seen, a->target)) {
				switch(a->operation) {
					case PU_LOG_OPERATION_INSTALL:
					case PU_LOG_OPERATION_REINSTALL:
					case PU_LOG_OPERATION_UPGRADE:
					case PU_LOG_OPERATION_DOWNGRADE:
						printf("%s %s\n", a->target, a->new_version);
						/* fall through */
					case PU_LOG_OPERATION_REMOVE:
						seen = alpm_list_add(seen, strdup(a->target));
						break;
				}
			}
			pu_log_action_free(a);
		}
		FREELIST(seen);
	} else if(!after && !before && !pkgs && !caller && !actions && !warnings && !commandline && !grep) {
		for(i = entries; i; i = i->next) {
			pu_log_entry_t *e = i->data;
			print_entry(stdout, e);
		}
	} else {
		for(i = entries; i; i = i->next) {
			pu_log_entry_t *e = i->data;

			if(after && mktime(&e->timestamp) >= after) {
				print_entry(stdout, e);
				continue;
			}

			if(before && mktime(&e->timestamp) <= before) {
				print_entry(stdout, e);
				continue;
			}

			if(caller) {
				const char *c = e->caller ? e->caller : "";
				if(alpm_list_find_str(caller, c)) {
					print_entry(stdout, e);
					continue;
				}
			}

			if(commandline && strncasecmp(e->message, "running ", 8) == 0) {
				print_entry(stdout, e);
				continue;
			}

			if(warnings) {
				if(strncmp(e->message, "error: ", 7) == 0
						|| strncmp(e->message, "warning: ", 9) == 0
						|| strncmp(e->message, "note: ", 6) == 0) {
					print_entry(stdout, e);
					continue;
				}
			}

			if(actions) {
				pu_log_action_t *a = pu_log_action_parse(e->message);
				if(a) {
					const char *op = NULL;
					switch(a->operation) {
						case PU_LOG_OPERATION_INSTALL:   op = "install";   break;
						case PU_LOG_OPERATION_REINSTALL: op = "reinstall"; break;
						case PU_LOG_OPERATION_UPGRADE:   op = "upgrade";   break;
						case PU_LOG_OPERATION_DOWNGRADE: op = "downgrade"; break;
						case PU_LOG_OPERATION_REMOVE:    op = "remove";    break;
					}
					pu_log_action_free(a);
					if(alpm_list_find_str(actions, "all")
							|| alpm_list_find_str(actions, op)) {
						print_entry(stdout, e);
						continue;
					}
				}
			}

			if(grep) {
				alpm_list_t *j;
				for(j = grep; j; j = alpm_list_next(j)) {
					if(regexec(j->data, e->message, 0, NULL, 0) == 0) {
						print_entry(stdout, e);
						break;
					}
				}
				if(j) {
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
	FREELIST(actions);
	FREELIST(caller);
	alpm_list_free_inner(entries, (alpm_list_fn_free) pu_log_entry_free);
	alpm_list_free(entries);
	alpm_list_free_inner(grep, (alpm_list_fn_free) regfree);
	FREELIST(grep);
	free(logfile);
	return ret;
}

/* vim: set ts=2 sw=2 noet: */
