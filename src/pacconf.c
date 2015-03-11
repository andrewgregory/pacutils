#include <getopt.h>

#include <pacutils.h>

const char *myname = "pacconf", *myver = "0.2";

pu_config_t *config = NULL;
alpm_list_t *directives = NULL;
char sep = '\n', *repo_name = NULL;
int repo_list = 0, verbose = 0;

enum {
	FLAG_ARCH = 1000,
	FLAG_CONFIG,
	FLAG_HELP,
	FLAG_REPO,
	FLAG_REPO_LIST,
	FLAG_ROOT,
	FLAG_VERBOSE,
	FLAG_VERSION,
};

void cleanup(void)
{
	alpm_list_free(directives);
	pu_config_free(config);
}

void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
#define hputs(x) fputs(x"\n", stream)
	hputs("pacconf - query pacman's configuration file");
	hputs("usage:  pacconf [options] <directive>...");
	hputs("        pacconf (--repo-list|--help|--version)");
	hputs("options:");
	hputs("  --arch=<arch>    set an alternate architecture");
	hputs("  --config=<path>  set an alternate configuration file");
	hputs("  --help           display this help information");
	hputs("  --repo-list      list configured repositories");
	hputs("  --repo=<remote>  query options for a specific repo");
	hputs("  --root=<path>    set an alternate installation root");
	hputs("  --verbose        always show directive names");
	hputs("  --version        display version information");
#undef hputs
	cleanup();
	exit(ret);
}

void parse_opts(int argc, char **argv)
{
	char *config_file = "/etc/pacman.conf";
	pu_config_t *argv_config = pu_config_new();
	int c;

	char *short_opts = "";
	struct option long_opts[] = {
		{ "arch"      , required_argument , NULL , FLAG_ARCH      },
		{ "config"    , required_argument , NULL , FLAG_CONFIG    },
		{ "help"      , no_argument       , NULL , FLAG_HELP      },
		{ "repo"      , required_argument , NULL , FLAG_REPO      },
		{ "repo-list" , no_argument       , NULL , FLAG_REPO_LIST },
		{ "root"      , required_argument , NULL , FLAG_ROOT      },
		{ "verbose"   , no_argument       , NULL , FLAG_VERBOSE   },
		{ "version"   , no_argument       , NULL , FLAG_VERSION   },
		{ 0, 0, 0, 0 },
	};

	while((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch(c) {
			case FLAG_ARCH:
				free(argv_config->architecture);
				argv_config->architecture = strdup(optarg);
				break;
			case FLAG_CONFIG:
				config_file = optarg;
				break;
			case FLAG_HELP:
				usage(0);
				break;
			case FLAG_REPO:
				repo_name = optarg;
				break;
			case FLAG_REPO_LIST:
				repo_list = 1;
				break;
			case FLAG_ROOT:
				free(argv_config->rootdir);
				argv_config->rootdir = strdup(optarg);
				break;
			case FLAG_VERBOSE:
				verbose = 1;
				break;
			case FLAG_VERSION:
				pu_print_version(myname, myver);
				cleanup();
				exit(0);
				break;
			case '?':
			default:
				usage(1);
				break;
		}
	}

	if((config = pu_ui_config_parse(argv_config, config_file)) == NULL) {
		fprintf(stderr, "error parsing '%s'\n", config_file);
	}
}

void list_repos(void)
{
	alpm_list_t *r;
	for(r = config->repos; r; r = r->next) {
		pu_repo_t *repo = r->data;
		if(!repo_name || strcmp(repo->name, repo_name) == 0) {
			printf("%s%c", repo->name, sep);
		}
	}
}

void show_float(const char *directive, float val)
{
	if(verbose) {
		printf("%s = ", directive);
	}
	printf("%f%c", val, sep);
}

void show_bool(const char *directive, short unsigned int val)
{
	if(val) {
		printf("%s%c", directive, sep);
	}
}

void show_str(const char *directive, const char *val)
{
	if(!val) {
		return;
	}
	if(verbose) {
		printf("%s = ", directive);
	}
	printf("%s%c", val, sep);
}

void show_list_str(const char *directive, alpm_list_t *list)
{
	alpm_list_t *i;
	for(i = list; i; i = i->next) {
		show_str(directive, i->data);
	}
}

void show_cleanmethod(const char *directive, unsigned int method)
{
	if(method & PU_CONFIG_CLEANMETHOD_KEEP_INSTALLED) {
		show_str(directive, "KeepInstalled");
	}
	if(method & PU_CONFIG_CLEANMETHOD_KEEP_CURRENT) {
		show_str(directive, "KeepCurrent");
	}
}

void show_siglevel(const char *directive, alpm_siglevel_t level, int pkgonly)
{
	if(level == ALPM_SIG_USE_DEFAULT) {
		return;
	}

	if(level & ALPM_SIG_PACKAGE) {
		if(level & ALPM_SIG_PACKAGE_OPTIONAL) {
			show_str(directive, "PackageOptional");
		} else {
			show_str(directive, "PackageRequired");
		}

		if(level & ALPM_SIG_PACKAGE_UNKNOWN_OK) {
			show_str(directive, "PackageTrustAll");
		} else {
			show_str(directive, "PackageTrustedOnly");
		}
	} else {
		show_str(directive, "PackageNever");
	}

	if(pkgonly) {
		return;
	}

	if(level & ALPM_SIG_DATABASE) {
		if(level & ALPM_SIG_DATABASE_OPTIONAL) {
			show_str(directive, "DatabaseOptional");
		} else {
			show_str(directive, "DatabaseRequired");
		}

		if(level & ALPM_SIG_DATABASE_UNKNOWN_OK) {
			show_str(directive, "DatabaseTrustAll");
		} else {
			show_str(directive, "DatabaseTrustedOnly");
		}
	} else {
		show_str(directive, "DatabaseNever");
	}
}

void show_usage(const char *directive, alpm_db_usage_t usage)
{
	if(usage & ALPM_DB_USAGE_ALL) {
		show_str(directive, "All");
	} else {
		if(usage & ALPM_DB_USAGE_SYNC) {
			show_str(directive, "Sync");
		}
		if(usage & ALPM_DB_USAGE_SEARCH) {
			show_str(directive, "Search");
		}
		if(usage & ALPM_DB_USAGE_INSTALL) {
			show_str(directive, "Install");
		}
		if(usage & ALPM_DB_USAGE_UPGRADE) {
			show_str(directive, "Upgrade");
		}
	}
}

void dump_repo(pu_repo_t *repo)
{
	show_usage("Usage", repo->usage);
	show_siglevel("SigLevel", repo->siglevel, 0);
	show_list_str("Server", repo->servers);
}

void dump_config(void)
{
	alpm_list_t *i;

	printf("[options]%c", sep);

	show_str("RootDir", config->rootdir);
	show_str("DBPath", config->dbpath);
	show_list_str("CacheDir", config->cachedirs);
	show_str("GPGDir", config->gpgdir);
	show_str("LogFile", config->logfile);

	show_list_str("HoldPkg", config->holdpkgs);
	show_list_str("IgnorePkg", config->ignorepkgs);
	show_list_str("IgnoreGroup", config->ignoregroups);
	show_list_str("NoUpgrade", config->noupgrade);
	show_list_str("NoExtract", config->noupgrade);

	show_str("Architecture", config->architecture);
	show_str("XferCommand", config->xfercommand);

	show_bool("UseSyslog", config->usesyslog);
	show_bool("Color", config->color);
	show_bool("TotalDownload", config->totaldownload);
	show_bool("CheckSpace", config->checkspace);
	show_bool("VerbosePkgLists", config->verbosepkglists);
	show_bool("ILoveCandy", config->ilovecandy);

	show_float("UseDelta", config->usedelta);

	show_cleanmethod("CleanMethod", config->cleanmethod);

	show_siglevel("SigLevel", config->siglevel, 0);
	show_siglevel("LocalFileSigLevel", config->localfilesiglevel, 1);
	show_siglevel("RemoteFileSigLevel", config->remotefilesiglevel, 1);

	for(i = config->repos; i; i = i->next) {
		pu_repo_t *repo = i->data;
		printf("[%s]%c", repo->name, sep);
		dump_repo(repo);
	}
}

int list_repo_directives(alpm_list_t *directives)
{
	int ret = 0;
	alpm_list_t *i;
	pu_repo_t *repo = NULL;

	for(i = config->repos; i; i = i->next) {
		if(strcmp(repo_name, ((pu_repo_t*) i->data)->name) == 0) {
			repo = i->data;
			break;
		}
	}

	if(!repo) {
		fprintf(stderr, "error: repo '%s' not configured\n", repo_name);
		return 1;
	}

	if(!directives) {
		dump_repo(repo);
		return 0;
	}

	for(i = directives; i; i = i->next) {
		if(strcasecmp(i->data, "Server") == 0) {
			show_list_str("Server", repo->servers);
		} else if(strcasecmp(i->data, "SigLevel") == 0) {
			show_siglevel("SigLevel", repo->siglevel, 0);
		} else if(strcasecmp(i->data, "Usage") == 0) {
			show_usage("Usage", repo->usage);
		} else if(strcasecmp(i->data, "Include") == 0) {
			fputs("warning: 'Include' directives cannot be queried\n", stderr);
			ret = 1;
		} else {
			fprintf(stderr, "warning: unknown directive '%s'\n", (char*) i->data);
			ret = 1;
		}
	}

	return ret;
}

int list_directives(alpm_list_t *directives)
{
	int ret = 0;
	alpm_list_t *i;

	if(!directives) {
		dump_config();
		return 0;
	}

	for(i = directives; i; i = i->next) {
		if(strcasecmp(i->data, "RootDir") == 0) {
			show_str("RootDir", config->rootdir);
		} else if(strcasecmp(i->data, "DBPath") == 0) {
			show_str("DBPath", config->dbpath);
		} else if(strcasecmp(i->data, "CacheDir") == 0) {
			show_list_str("CacheDir", config->cachedirs);
		} else if(strcasecmp(i->data, "GPGDir") == 0) {
			show_str("GPGDir", config->gpgdir);
		} else if(strcasecmp(i->data, "LogFile") == 0) {
			show_str("LogFile", config->logfile);

		} else if(strcasecmp(i->data, "HoldPkg") == 0) {
			show_list_str("HoldPkg", config->holdpkgs);
		} else if(strcasecmp(i->data, "IgnorePkg") == 0) {
			show_list_str("IgnorePkg", config->ignorepkgs);
		} else if(strcasecmp(i->data, "IgnoreGroup") == 0) {
			show_list_str("IgnoreGroup", config->ignoregroups);
		} else if(strcasecmp(i->data, "NoUpgrade") == 0) {
			show_list_str("NoUpgrade", config->noupgrade);
		} else if(strcasecmp(i->data, "NoExtract") == 0) {
			show_list_str("NoExtract", config->noupgrade);


		} else if(strcasecmp(i->data, "Architecture") == 0) {
			show_str("Architecture", config->architecture);
		} else if(strcasecmp(i->data, "XferCommand") == 0) {
			show_str("XferCommand", config->xfercommand);

		} else if(strcasecmp(i->data, "UseSyslog") == 0) {
			show_bool("UseSyslog", config->usesyslog);
		} else if(strcasecmp(i->data, "Color") == 0) {
			show_bool("Color", config->color);
		} else if(strcasecmp(i->data, "TotalDownload") == 0) {
			show_bool("TotalDownload", config->totaldownload);
		} else if(strcasecmp(i->data, "CheckSpace") == 0) {
			show_bool("CheckSpace", config->checkspace);
		} else if(strcasecmp(i->data, "VerbosePkgLists") == 0) {
			show_bool("VerbosePkgLists", config->verbosepkglists);

		} else if(strcasecmp(i->data, "UseDelta") == 0) {
			show_float("UseDelta", config->usedelta);

		} else if(strcasecmp(i->data, "CleanMethod") == 0) {
			show_cleanmethod("CleanMethod", config->cleanmethod);

		} else if(strcasecmp(i->data, "SigLevel") == 0) {
			show_siglevel("SigLevel", config->siglevel, 0);
		} else if(strcasecmp(i->data, "LocalFileSigLevel") == 0) {
			show_siglevel("LocalFileSigLevel", config->localfilesiglevel, 1);
		} else if(strcasecmp(i->data, "RemoteFileSigLevel") == 0) {
			show_siglevel("RemoteFileSigLevel", config->remotefilesiglevel, 1);

		} else if(strcasecmp(i->data, "Include") == 0) {
			fputs("warning: 'Include' directives cannot be queried\n", stderr);
			ret = 1;
		} else {
			fprintf(stderr, "warning: unknown directive '%s'\n", (char*) i->data);
			ret = 1;
		}
	}

	return ret;
}

int main(int argc, char **argv)
{
	int ret = 0;

	parse_opts(argc, argv);
	if(!config) {
		ret = 1;
		goto cleanup;
	}

	for(; optind < argc; optind++) {
		directives = alpm_list_add(directives, argv[optind]);
	}

	if(alpm_list_count(directives) != 1) {
		verbose = 1;
	}

	if(repo_list) {
		if(directives) {
			fputs("error: directives may not be specified with --repo-list\n", stderr);
			ret = 1;
			goto cleanup;
		}
		list_repos();
	} else if(repo_name) {
		ret = list_repo_directives(directives);
	} else {
		ret = list_directives(directives);
	}

cleanup:
	cleanup();

	return ret;
}

/* vim: set ts=2 sw=2 noet: */
