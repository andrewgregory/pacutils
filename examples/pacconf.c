#include <getopt.h>

#include <pacutils.h>

pu_config_t *config = NULL;
char sep = '\n', *repo_name = NULL;
int repo_list = 0, verbose = 0;

// TODO exit codes

void cleanup(void)
{
	free(repo_name);
	pu_config_free(config);
}

void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
	fputs("pacconf - query pacman's configuration file\n", stream);
	fputs("usage:  pacconf [options] <directive>...\n", stream);
	fputs("        pacconf (--repo-list|--help|--version)\n", stream);
	fputs("options:\n", stream);
	fputs("  --config=<path>  set an alternate configuration file\n", stream);
	fputs("  --repo=<remote>  query options for a specific repo\n", stream);
	fputs("  --verbose        always show directive names\n", stream);
	fputs("  --repo-list      list configured repositories\n", stream);
	fputs("  --help           display this help information\n", stream);
	fputs("  --version        display version information\n", stream);
	exit(ret);
}

void version(void)
{
	printf("pacconf v0.1\n");
	exit(0);
}

pu_config_t *parse_opts(int argc, char **argv)
{
	char *config_file = strdup("/etc/pacman.conf");
	pu_config_t *config = NULL;
	int c;

	char *short_opts = "";
	struct option long_opts[] = {
		{ "config"    , required_argument , NULL , 'c' },
		{ "repo"      , required_argument , NULL , 'r' },
		{ "repo-list" , no_argument       , NULL , 'l' },
		{ "verbose"   , no_argument       , NULL , 'v' },
		{ "help"      , no_argument       , NULL , 'h' },
		{ "version"   , no_argument       , NULL , 'V' },
		{ 0, 0, 0, 0 },
	};

	/* process remaining command-line options */
	c = getopt_long(argc, argv, short_opts, long_opts, NULL);
	while(c != -1) {
		switch(c) {
			case 'c':
				free(config_file);
				config_file = strdup(optarg);
				break;
			case 'l':
				repo_list = 1;
				break;
			case 'r':
				repo_name = strdup(optarg);
				break;
			case 'v':
				verbose = 1;
				break;
			case 'h':
				usage(0);
				break;
			case 'V':
				version();
				break;
			case '?':
			default:
				usage(1);
				break;
		}
		c = getopt_long(argc, argv, short_opts, long_opts, NULL);
	}

	config = pu_config_new_from_file(config_file);
	if(!config) {
		fprintf(stderr, "error parsing '%s'\n", config_file);
	}
	free(config_file);

	return config;
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
	// TODO
}

void show_siglevel(const char *directive, alpm_siglevel_t siglevel)
{
	// TODO
}

void dump_repo(pu_repo_t *repo)
{
	show_list_str("Server", repo->servers);
	show_siglevel("SigLevel", repo->siglevel);
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

	show_float("UseDelta", config->usedelta);

	show_cleanmethod("CleanMethod", config->cleanmethod);

	show_siglevel("SigLevel", config->siglevel);
	show_siglevel("LocalFileSigLevel", config->localfilesiglevel);
	show_siglevel("RemoteFileSigLevel", config->remotefilesiglevel);

	for(i = config->repos; i; i = i->next) {
		pu_repo_t *repo = i->data;
		printf("[%s]%c", repo->name, sep);
		dump_repo(repo);
	}
}

void list_repo_directives(alpm_list_t *directives)
{
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
		return;
	}

	if(!directives) {
		dump_repo(repo);
		return;
	}

	for(i = directives; i; i = i->next) {
		if(strcasecmp(i->data, "Server") == 0) {
			show_list_str("Server", repo->servers);
		} else if(strcasecmp(i->data, "SigLevel") == 0) {
			show_siglevel("SigLevel", repo->siglevel);
		} else {
			fprintf(stderr, "warning: unknown directive '%s'\n", (char*) i->data);
		}
	}
}

void list_directives(alpm_list_t *directives)
{
	alpm_list_t *i;

	if(!directives) {
		dump_config();
		return;
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
			show_siglevel("SigLevel", config->siglevel);
		} else if(strcasecmp(i->data, "LocalFileSigLevel") == 0) {
			show_siglevel("LocalFileSigLevel", config->localfilesiglevel);
		} else if(strcasecmp(i->data, "RemoteFileSigLevel") == 0) {
			show_siglevel("RemoteFileSigLevel", config->remotefilesiglevel);

		} else if(strcasecmp(i->data, "Include") == 0) {
			fputs("warning: 'Include' directives cannot be queried\n", stderr);

		} else {
			fprintf(stderr, "warning: unknown directive '%s'\n", (char*) i->data);
		}
	}
}

int main(int argc, char **argv)
{
	alpm_list_t *directives = NULL;
	int ret = 0;

	config = parse_opts(argc, argv);
	if(!config) {
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
			goto cleanup;
		}
		list_repos();
	} else if(repo_name) {
		list_repo_directives(directives);
	} else {
		list_directives(directives);
	}

cleanup:
	cleanup();
	alpm_list_free(directives);

	return ret;
}

/* vim: set ts=2 sw=2 noet: */
