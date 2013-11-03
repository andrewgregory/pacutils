#define _GNU_SOURCE /* strcasestr */

#include <getopt.h>
#include <regex.h>

#include <pacutils.h>

const char *myname = "pacsift", *myver = "0.1";

pu_config_t *config = NULL;
alpm_handle_t *handle = NULL;
alpm_loglevel_t log_level = ALPM_LOG_ERROR | ALPM_LOG_WARNING;

int invert = 0, re = 0, exact = 0, or = 0;
char sep = '\n';
alpm_list_t *search_dbs = NULL;
alpm_list_t *repo, *name, *description, *packager;
alpm_list_t *group, *license;
alpm_list_t *ownsfile;
alpm_list_t *requiredby;
alpm_list_t *provides, *dependson, *conflicts, *replaces;

typedef const char* (str_accessor) (alpm_pkg_t* pkg);
typedef alpm_list_t* (str_list_accessor) (alpm_pkg_t* pkg);

enum longopt_flags {
	FLAG_CONFIG = 1000,
	FLAG_DBPATH,
	FLAG_DEBUG,
	FLAG_HELP,
	FLAG_ROOT,
	FLAG_VERSION,

	FLAG_NAME,
	FLAG_DESCRIPTION,
	FLAG_OWNSFILE,
	FLAG_PACKAGER,
	FLAG_REPO,
};

void cleanup(int ret)
{
	alpm_list_free(search_dbs);
	alpm_release(handle);
	pu_config_free(config);

	FREELIST(repo);
	FREELIST(name);
	FREELIST(description);
	FREELIST(packager);

	FREELIST(group);
	FREELIST(license);

	exit(ret);
}

const char *get_dbname(alpm_pkg_t *pkg)
{
	return alpm_db_get_name(alpm_pkg_get_db(pkg));
}

int ptr_cmp(const void *p1, const void *p2)
{
	return p2 - p1;
}

/* regcmp wrapper with error handling */
void _regcomp(regex_t *preg, const char *regex, int cflags)
{
	int err;
	if((err = regcomp(preg, regex, REG_EXTENDED | REG_ICASE | REG_NOSUB)) != 0) {
		char errstr[100];
		regerror(err, preg, errstr, 100);
		fprintf(stderr, "error: invalid regex '%s' (%s)\n", regex,  errstr);
		cleanup(1);
	}
}

alpm_list_t *filter_filelist(alpm_list_t **pkgs, const char *str)
{
	alpm_list_t *p, *matches = NULL;
	if(re) {
		regex_t preg;
		_regcomp(&preg, str, REG_EXTENDED | REG_ICASE | REG_NOSUB);
		for(p = *pkgs; p; p = p->next) {
			alpm_filelist_t *files = alpm_pkg_get_files(p->data);
			int i;
			for(i = 0; i < files->count; ++i) {
				if(regexec(&preg, files->files[i].name, 0, NULL, 0) == 0) {
					matches = alpm_list_add(matches, p->data);
					break;
				}
			}
		}
		regfree(&preg);
	} else if (exact) {
		for(p = *pkgs; p; p = p->next) {
			if(alpm_filelist_contains(alpm_pkg_get_files(p->data), str)) {
				matches = alpm_list_add(matches, p->data);
			}
		}
	} else {
		for(p = *pkgs; p; p = p->next) {
			alpm_filelist_t *files = alpm_pkg_get_files(p->data);
			int i;
			for(i = 0; i < files->count; ++i) {
				if(strcasestr(files->files[i].name, str)) {
					matches = alpm_list_add(matches, p->data);
					break;
				}
			}
		}
	}
	for(p = matches; p; p = p->next) {
		*pkgs = alpm_list_remove(*pkgs, p->data, ptr_cmp, NULL);
	}
	return matches;
}

alpm_list_t *filter_str(alpm_list_t **pkgs, const char *str, str_accessor *func)
{
	alpm_list_t *p, *matches = NULL;
	if(re) {
		regex_t preg;
		_regcomp(&preg, str, REG_EXTENDED | REG_ICASE | REG_NOSUB);
		for(p = *pkgs; p; p = p->next) {
			const char *s = func(p->data);
			if(s && regexec(&preg, s, 0, NULL, 0) == 0) {
				matches = alpm_list_add(matches, p->data);
			}
		}
		regfree(&preg);
	} else if(exact) {
		for(p = *pkgs; p; p = p->next) {
			const char *s = func(p->data);
			if(s && strcasecmp(s, str) == 0) {
				matches = alpm_list_add(matches, p->data);
			}
		}
	} else {
		for(p = *pkgs; p; p = p->next) {
			const char *s = func(p->data);
			if(s && strcasestr(s, str)) {
				matches = alpm_list_add(matches, p->data);
			}
		}
	}
	for(p = matches; p; p = p->next) {
		*pkgs = alpm_list_remove(*pkgs, p->data, ptr_cmp, NULL);
	}
	return matches;
}

alpm_list_t *filter_pkgs(alpm_list_t *pkgs)
{
	alpm_list_t *i, *matches = NULL, *haystack = alpm_list_copy(pkgs);

	if(name) {
		for(i = name; i; i = i->next) {
			matches = alpm_list_join(matches, filter_str(&haystack, i->data, alpm_pkg_get_name));
		}
		if(!or) {
			alpm_list_free(haystack);
			haystack = matches;
			matches = NULL;
		}
	}

	if(description) {
		for(i = description; i; i = i->next) {
			matches = alpm_list_join(matches, filter_str(&haystack, i->data, alpm_pkg_get_desc));
		}
		if(!or) {
			alpm_list_free(haystack);
			haystack = matches;
			matches = NULL;
		}
	}

	if(packager) {
		for(i = packager; i; i = i->next) {
			matches = alpm_list_join(matches, filter_str(&haystack, i->data, alpm_pkg_get_packager));
		}
		if(!or) {
			alpm_list_free(haystack);
			haystack = matches;
			matches = NULL;
		}
	}

	if(repo) {
		for(i = repo; i; i = i->next) {
			matches = alpm_list_join(matches, filter_str(&haystack, i->data, get_dbname));
		}
		if(!or) {
			alpm_list_free(haystack);
			haystack = matches;
			matches = NULL;
		}
	}

	if(ownsfile) {
		for(i = ownsfile; i; i = i->next) {
			matches = alpm_list_join(matches, filter_filelist(&haystack, i->data));
		}
		if(!or) {
			alpm_list_free(haystack);
			haystack = matches;
			matches = NULL;
		}
	}

	if(invert) {
		matches = alpm_list_diff(pkgs, haystack, ptr_cmp);
		alpm_list_free(haystack);
		return matches;
	} else {
		return haystack;
	}
}

void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
#define hputs(str) fputs(str"\n", stream);
	hputs("pacsift - query packages");
	hputs("usage:  pacsift [options] (<field> <term>)...");
	hputs("        pacsift (--help|--version)");
	hputs("options:");
	hputs("   --config=<path>     set an alternate configuration file");
	hputs("   --dbpath=<path>     set an alternate database location");
	hputs("   --help              display this help information");
	hputs("   --version           display version information");

	hputs("   --invert            display packages which DO NOT match search criteria");
	/*hputs("   --or                OR search terms instead of AND");*/

	hputs("   --exact");
	hputs("   --regex");

	hputs(" Filters:");
	hputs("   Note: filters are unaffected by --invert and --or");
	hputs("   --local             search installed packages");
	hputs("   --sync              search packages in all sync repositories");
	/*hputs("   --depends           limit to packages installed as dependencies");*/
	/*hputs("   --explicit          limit to packages installed explicitly");*/
	/*hputs("   --unrequired        limit to unrequired packages");*/
	/*hputs("   --required          limit to required packages");*/
	/*hputs("   --foreign           limit to packages not in a sync repo");*/
	/*hputs("   --native            limit to packages in a sync repo");*/

	hputs(" Package Fields:");
	hputs("   Note: options specified multiple times will be OR'd");
	hputs("   --repo=<name>       search packages in repo <name>");
	hputs("   --name=<name>");
	hputs("   --description=<desc>");
	hputs("   --packager=<name>");
	hputs("   --group=<name>      search packages in group <name>");
	hputs("   --owns-file=<path>  search packages that own <path>");
	/*hputs("   --license");*/
	/*hputs("   --provides");*/
	/*hputs("   --depends-on");*/
	/*hputs("   --required-by");*/
	/*hputs("   --conflicts");*/
	/*hputs("   --replaces");*/
#undef hputs

	cleanup(ret);
}

pu_config_t *parse_opts(int argc, char **argv)
{
	char *config_file = "/etc/pacman.conf";
	pu_config_t *config = NULL;
	int c;

	char *short_opts = "";
	struct option long_opts[] = {
		{ "config"        , required_argument , NULL    , FLAG_CONFIG        } ,
		{ "dbpath"        , required_argument , NULL    , FLAG_DBPATH        } ,
		{ "debug"         , no_argument       , NULL    , FLAG_DEBUG         } ,
		{ "help"          , no_argument       , NULL    , FLAG_HELP          } ,
		{ "version"       , no_argument       , NULL    , FLAG_VERSION       } ,


		{ "invert"        , no_argument       , &invert , 1                  } ,
		{ "regex"         , no_argument       , &re     , 1                  } ,
		{ "exact"         , no_argument       , &exact  , 1                  } ,

		{ "repo"          , required_argument , NULL    , FLAG_REPO          } ,
		{ "packager"      , required_argument , NULL    , FLAG_PACKAGER      } ,
		{ "name"          , required_argument , NULL    , FLAG_NAME          } ,
		{ "description"   , required_argument , NULL    , FLAG_DESCRIPTION   } ,
		{ "owns-file"     , required_argument , NULL    , FLAG_OWNSFILE      } ,

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
			case 0:
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
			case FLAG_HELP:
				usage(0);
				break;
			case FLAG_VERSION:
				pu_print_version(myname, myver);
				cleanup(0);
				break;

			case FLAG_REPO:
				repo = alpm_list_add(repo, strdup(optarg));
				break;
			case FLAG_NAME:
				name = alpm_list_add(name, strdup(optarg));
				break;
			case FLAG_PACKAGER:
				packager = alpm_list_add(packager, strdup(optarg));
				break;
			case FLAG_DESCRIPTION:
				description = alpm_list_add(description, strdup(optarg));
				break;
			case FLAG_OWNSFILE:
				ownsfile = alpm_list_add(ownsfile, strdup(optarg));
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

void parse_pkg_spec(char *spec, char **pkgname, char **dbname)
{
	char *c;
	if((c = strchr(spec, '/'))) {
		*c = '\0';
		*pkgname = c + 1;
		*dbname = spec;
	} else {
		*dbname = NULL;
		*pkgname = spec;
	}
}

alpm_db_t *find_db(const char *dbname) {
	alpm_list_t *s;
	if(strcmp(dbname, "local") == 0) {
		return alpm_get_localdb(handle);
	}
	for(s = alpm_get_syncdbs(handle); s; s = s->next) {
		if(strcmp(dbname, alpm_db_get_name(s->data)) == 0) {
			return s->data;
		}
	}
	return NULL;
}

void find_pkg(alpm_list_t **pkgs, char *pkgspec) {
	char *pkgname, *dbname;
	int found = 0;
	parse_pkg_spec(pkgspec, &pkgname, &dbname);
	if(dbname) {
		alpm_db_t *db = find_db(dbname);
		if(db) {
			alpm_pkg_t *pkg = alpm_db_get_pkg(db, pkgname);
			if(pkg) {
				*pkgs = alpm_list_add(*pkgs, pkg);
				found = 1;
			}
		}
	} else {
		alpm_db_t *db = alpm_get_localdb(handle);
		alpm_pkg_t *p = alpm_db_get_pkg(db, pkgname);
		alpm_list_t *d;
		if(p) {
			found = 1;
			*pkgs = alpm_list_add(*pkgs, p);
		}
		for(d = alpm_get_syncdbs(handle); d; d = d->next) {
			alpm_pkg_t *p = alpm_db_get_pkg(d->data, pkgname);
			if(p) {
				found = 1;
				*pkgs = alpm_list_add(*pkgs, p);
			}
		}
	}

	if(!found) {
		printf("warning: could not locate pkg '%s'\n", pkgname);
	}
}

int main(int argc, char **argv)
{
	alpm_list_t *sync_dbs = NULL, *i;
	int ret = 0;

	if(!(config = parse_opts(argc, argv))) {
		goto cleanup;
	}

	if(!(handle = pu_initialize_handle_from_config(config))) {
		fprintf(stderr, "error: failed to initialize alpm.\n");
		ret = 1;
		goto cleanup;
	}

	sync_dbs = pu_register_syncdbs(handle, config->repos);
	if(!sync_dbs) {
		fprintf(stderr, "error: no valid sync dbs configured.\n");
		ret = 1;
		goto cleanup;
	}

	alpm_list_t *haystack = NULL;
	if(!isatty(fileno(stdin)) && !feof(stdin)) {
		char buf[256];
		while(fgets(buf, 256, stdin)) {
			char *c = strchr(buf, '\n');
			if(c) *c = '\0';
			find_pkg(&haystack, buf);
		}
	} else {
		alpm_list_t *p, *s;
		for(p = alpm_db_get_pkgcache(alpm_get_localdb(handle)); p; p = p->next) {
			haystack = alpm_list_add(haystack, p->data);
		}
		for(s = alpm_get_syncdbs(handle); s; s = s->next) {
			for(p = alpm_db_get_pkgcache(s->data); p; p = p->next) {
				haystack = alpm_list_add(haystack, p->data);
			}
		}
	}

	alpm_list_t *matches = filter_pkgs(haystack);
	for(i = matches; i; i = i->next) {
		printf("%s/%s\n", get_dbname(i->data), alpm_pkg_get_name(i->data));
	}
	alpm_list_free(matches);

cleanup:
	cleanup(ret);

	return 0;
}

/* vim: set ts=2 sw=2 noet: */
