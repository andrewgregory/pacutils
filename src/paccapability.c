#include <getopt.h>

#include <pacutils.h>

#include "config-defaults.h"

const char *myname = "paccapability", *myver = BUILDVER;

enum longopt_flags {
	FLAG_HELP = 1000,
	FLAG_VERSION,
};

const char *cap_prefix = "ALPM_CAPABILITY_";

struct cap {
	const char *name;
	enum alpm_caps value;
}  cap_map[] = {
	{ "NLS", ALPM_CAPABILITY_NLS },
	{ "DOWNLOADER", ALPM_CAPABILITY_DOWNLOADER },
	{ "SIGNATURES", ALPM_CAPABILITY_SIGNATURES },
};

void usage(int ret) {
	FILE *stream = (ret ? stderr : stdout);
#define hputs(x) fputs(x"\n", stream)
	hputs("paccapability - query alpm capabilities");
	hputs("usage:  paccapability [options] [<capability>...]");
	hputs("        paccapability (--help|--version)");
	hputs("options:");
	hputs("  --help           display this help information");
	hputs("  --version        display version information");
#undef hputs
	exit(ret);
}

struct cap *get_cap(const char *name) {
	size_t i;

	if(strncasecmp(name, cap_prefix, strlen(cap_prefix)) == 0) {
		name += strlen(cap_prefix);
	}

	for(i = 0; i < sizeof(cap_map) / sizeof(struct cap); i++) {
		if(strcasecmp(name, cap_map[i].name) == 0) { return &cap_map[i]; }
	}

	return NULL;
}

void parse_opts(int argc, char **argv) {
	int c;

	char *short_opts = "";
	struct option long_opts[] = {
		{ "help"      , no_argument       , NULL , FLAG_HELP    },
		{ "version"   , no_argument       , NULL , FLAG_VERSION },
		{ 0, 0, 0, 0 },
	};

	while((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch(c) {
			case FLAG_HELP:
				usage(0);
				break;
			case FLAG_VERSION:
				pu_print_version(myname, myver);
				exit(0);
				break;
			case '?':
			default:
				usage(1);
				break;
		}
	}
}

int main(int argc, char **argv) {
	int ret = 0;

	parse_opts(argc, argv);

	if(optind < argc) {
		for(; optind < argc; optind++) {
			struct cap *c = get_cap(argv[optind]);
			if(c) {
				int hascap = (alpm_capabilities() & c->value) ? 1 : 0;
				printf("%s%s: %d\n", cap_prefix, c->name, hascap);
				if(!hascap) { ret = 1; }
			} else {
				pu_ui_warn("unknown capability '%s'", argv[optind]);
				ret = 1;
			}
		}
	} else {
		size_t i;
		for(i = 0; i < sizeof(cap_map) / sizeof(struct cap); i++) {
			if(alpm_capabilities() & cap_map[i].value) {
				printf("%s%s\n", cap_prefix, cap_map[i].name);
			}
		}
	}

	return ret;
}

/* vim: set ts=2 sw=2 noet: */
