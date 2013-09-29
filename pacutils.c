#include "pacutils.h"

#include <sys/time.h>
#include <sys/utsname.h>

enum _pu_setting_name {
	PU_CONFIG_OPTION_ROOTDIR,
	PU_CONFIG_OPTION_DBPATH,
	PU_CONFIG_OPTION_GPGDIR,
	PU_CONFIG_OPTION_LOGFILE,
	PU_CONFIG_OPTION_ARCHITECTURE,
	PU_CONFIG_OPTION_XFERCOMMAND,

	PU_CONFIG_OPTION_CLEANMETHOD,
	PU_CONFIG_OPTION_USESYSLOG,
	PU_CONFIG_OPTION_USEDELTA,
	PU_CONFIG_OPTION_TOTALDOWNLOAD,
	PU_CONFIG_OPTION_CHECKSPACE,
	PU_CONFIG_OPTION_VERBOSEPKGLISTS,

	PU_CONFIG_OPTION_SIGLEVEL,

	PU_CONFIG_OPTION_HOLDPKGS,
	PU_CONFIG_OPTION_IGNOREPKGS,
	PU_CONFIG_OPTION_IGNOREGROUPS,
	PU_CONFIG_OPTION_NOUPGRADE,
	PU_CONFIG_OPTION_NOEXTRACT,
	PU_CONFIG_OPTION_REPOS,
	PU_CONFIG_OPTION_CACHEDIRS,

	PU_CONFIG_OPTION_SERVER,

	PU_CONFIG_OPTION_INCLUDE
};

struct _pu_config_setting {
	char *name;
	unsigned short type;
} _pu_config_settings[] = {
	{"RootDir",         PU_CONFIG_OPTION_ROOTDIR},
	{"DBPath",          PU_CONFIG_OPTION_DBPATH},
	{"GPGDir",          PU_CONFIG_OPTION_GPGDIR},
	{"LogFile",         PU_CONFIG_OPTION_LOGFILE},
	{"Architecture",    PU_CONFIG_OPTION_ARCHITECTURE},
	{"XferCommand",     PU_CONFIG_OPTION_XFERCOMMAND},

	{"CleanMethod",     PU_CONFIG_OPTION_CLEANMETHOD},
	{"UseSyslog",       PU_CONFIG_OPTION_USESYSLOG},
	{"UseDelta",        PU_CONFIG_OPTION_USEDELTA},
	{"TotalDownload",   PU_CONFIG_OPTION_TOTALDOWNLOAD},
	{"CheckSpace",      PU_CONFIG_OPTION_CHECKSPACE},
	{"VerbosePkgLists", PU_CONFIG_OPTION_VERBOSEPKGLISTS},

	{"SigLevel",        PU_CONFIG_OPTION_SIGLEVEL},

	{"HoldPkg",         PU_CONFIG_OPTION_HOLDPKGS},
	{"IgnorePkg",       PU_CONFIG_OPTION_IGNOREPKGS},
	{"IgnoreGroup",     PU_CONFIG_OPTION_IGNOREGROUPS},
	{"NoUpgrade",       PU_CONFIG_OPTION_NOUPGRADE},
	{"NoExtract",       PU_CONFIG_OPTION_NOEXTRACT},
	{"CacheDir",        PU_CONFIG_OPTION_CACHEDIRS},

	{"Include",         PU_CONFIG_OPTION_INCLUDE},

	{"Server",          PU_CONFIG_OPTION_SERVER},

	{NULL, 0}
};

static struct _pu_config_setting *_pu_config_lookup_setting(const char *optname)
{
	int i;
	for(i = 0; _pu_config_settings[i].name; ++i) {
		if(strcmp(optname, _pu_config_settings[i].name) == 0) {
			return &_pu_config_settings[i];
		}
	}
	return NULL;
}

char *pu_strreplace(const char *str,
		const char *target, const char *replacement)
{
	int found = 0;
	const char *ptr;
	char *newstr;
	size_t tlen = strlen(target);
	size_t rlen = strlen(replacement);
	size_t newlen = strlen(str);

	/* count the number of occurrences */
	ptr = str;
	while((ptr = strstr(ptr, target))) {
		found++;
		ptr += tlen;
	}

	/* calculate the length of our new string */
	newlen += (found * (rlen - tlen));

	newstr = malloc(newlen + 1);
	newstr[0] = '\0';

	/* copy the string with the replacements */
	ptr = str;
	while((ptr = strstr(ptr, target))) {
		strncat(newstr, str, ptr - str);
		strcat(newstr, replacement);
		ptr += tlen;
		str = ptr;
	}
	strcat(newstr, str);

	return newstr;
}

size_t pu_strtrim(char *str)
{
	char *start = str, *end;

	if(!(str && *str)) {
		return 0;
	}

	end = str + strlen(str);

	for(; isspace((int) *start) && start < end; start++);
	for(; end > start && isspace((int) *(end - 1)); end--);

	*(end) = '\0';
	memmove(str, start, end - start + 1);

	return end - start;
}

struct pu_config_t *pu_config_new(void)
{
	return calloc(sizeof(struct pu_config_t), 1);
}

void pu_repo_free(pu_repo_t *repo)
{
	if(!repo) {
		return;
	}

	free(repo->name);
	FREELIST(repo->servers);

	free(repo);
}

struct pu_repo_t *pu_repo_new(void)
{
	return calloc(sizeof(struct pu_repo_t), 1);
}

void pu_config_free(pu_config_t *config)
{
	if(!config) {
		return;
	}

	free(config->rootdir);
	free(config->dbpath);
	free(config->logfile);
	free(config->gpgdir);
	free(config->architecture);
	free(config->xfercommand);

	FREELIST(config->holdpkgs);
	FREELIST(config->ignorepkgs);
	FREELIST(config->ignoregroups);
	FREELIST(config->noupgrade);
	FREELIST(config->noextract);
	FREELIST(config->cachedirs);

	alpm_list_free_inner(config->repos, (alpm_list_fn_free) pu_repo_free);
	alpm_list_free(config->repos);

	free(config);
}

int _pu_config_read_file(const char *filename, pu_config_t *config,
		pu_repo_t *repo)
{
	char buf[BUFSIZ];
	FILE *infile = fopen(filename, "r");

	if(!infile) {
		return -1;
	}

	while(fgets(buf, BUFSIZ, infile)) {
		char *ptr;
		size_t linelen;

		/* remove comments */
		if((ptr = strchr(buf, '#'))) {
			*ptr = '\0';
		}

		/* strip surrounding whitespace */
		linelen = pu_strtrim(buf);

		/* skip empty lines */
		if(buf[0] == '\0') {
			continue;
		}

		if(buf[0] == '[' && buf[linelen - 1] == ']') {
			buf[linelen - 1] = '\0';
			ptr = buf + 1;

			if(strcmp(ptr, "options") == 0) {
				repo = NULL;
			} else {
				repo = pu_repo_new();
				repo->name = strdup(ptr);
				config->repos = alpm_list_add(config->repos, repo);
			}
		} else {
			char *key = strtok_r(buf, " =", &ptr);
			char *val = strtok_r(NULL, " =", &ptr);
			struct _pu_config_setting *s = _pu_config_lookup_setting(key);

			if(!s) {
				printf("unknown directive '%s'\n", key);
				continue;
			}

			if(repo) {
				switch(s->type) {
					case PU_CONFIG_OPTION_INCLUDE:
						_pu_config_read_file(val, config, repo);
						break;
					case PU_CONFIG_OPTION_SIGLEVEL:
						/* TODO */
						break;
					case PU_CONFIG_OPTION_SERVER:
						repo->servers = alpm_list_add(repo->servers, strdup(val));
						break;
					default:
						/* TODO */
						break;
				}
			} else {
				switch(s->type) {
					case PU_CONFIG_OPTION_ROOTDIR:
						free(config->rootdir);
						config->rootdir = strdup(val);
						break;
					case PU_CONFIG_OPTION_DBPATH:
						free(config->dbpath);
						config->dbpath = strdup(val);
						break;
					case PU_CONFIG_OPTION_GPGDIR:
						free(config->gpgdir);
						config->gpgdir = strdup(val);
						break;
					case PU_CONFIG_OPTION_LOGFILE:
						free(config->logfile);
						config->logfile = strdup(val);
						break;
					case PU_CONFIG_OPTION_ARCHITECTURE:
						free(config->architecture);
						config->architecture = strdup(val);
						break;
					case PU_CONFIG_OPTION_XFERCOMMAND:
						free(config->xfercommand);
						config->xfercommand = strdup(val);
						break;
					case PU_CONFIG_OPTION_CLEANMETHOD:
						/* TODO */
						break;
					case PU_CONFIG_OPTION_USESYSLOG:
						config->usesyslog = 1;
						break;
					case PU_CONFIG_OPTION_USEDELTA:
						if(val) {
							char *end;
							float d = strtof(val, &end);
							if(*end != '\0' || d < 0.0 || d > 2.0) {
								/* TODO invalid delta ratio */
							} else {
								config->usedelta = d;
							}
						} else {
							config->usedelta = 0.7;
						}
						break;
					case PU_CONFIG_OPTION_TOTALDOWNLOAD:
						config->totaldownload = 1;
						break;
					case PU_CONFIG_OPTION_CHECKSPACE:
						config->checkspace = 1;
						break;
					case PU_CONFIG_OPTION_VERBOSEPKGLISTS:
						config->verbosepkglists = 1;
						break;
					case PU_CONFIG_OPTION_SIGLEVEL:
						/* TODO */
						break;
					case PU_CONFIG_OPTION_HOLDPKGS:
						while(val) {
							config->holdpkgs = alpm_list_add(config->holdpkgs, strdup(val));
							val = strtok_r(NULL, " ", &ptr);
						}
						break;
					case PU_CONFIG_OPTION_IGNOREPKGS:
						while(val) {
							config->ignorepkgs = alpm_list_add(config->ignorepkgs, strdup(val));
							val = strtok_r(NULL, " ", &ptr);
						}
						break;
					case PU_CONFIG_OPTION_IGNOREGROUPS:
						while(val) {
							config->ignorepkgs = alpm_list_add(config->ignorepkgs, strdup(val));
							val = strtok_r(NULL, " ", &ptr);
						}
						break;
					case PU_CONFIG_OPTION_NOUPGRADE:
						while(val) {
							config->noupgrade = alpm_list_add(config->noupgrade, strdup(val));
							val = strtok_r(NULL, " ", &ptr);
						}
						break;
					case PU_CONFIG_OPTION_NOEXTRACT:
						while(val) {
							config->noextract = alpm_list_add(config->noextract, strdup(val));
							val = strtok_r(NULL, " ", &ptr);
						}
						break;
					case PU_CONFIG_OPTION_CACHEDIRS:
						while(val) {
							config->cachedirs = alpm_list_add(config->cachedirs, strdup(val));
							val = strtok_r(NULL, " ", &ptr);
						}
						break;
					case PU_CONFIG_OPTION_INCLUDE:
						_pu_config_read_file(val, config, repo);
						break;
					default:
						/* TODO */
						break;
				}
			}
		}
	}
	fclose(infile);

	return 0;
}

void _pu_subst_server_vars(pu_config_t *config)
{
	alpm_list_t *r;
	for(r = config->repos; r; r = r->next) {
		pu_repo_t *repo = r->data;
		alpm_list_t *s;
		for(s = repo->servers; s; s = s->next) {
			char *url = pu_strreplace(s->data, "$repo", repo->name);
			s->data = pu_strreplace(url, "$arch", config->architecture);
			free(url);
		}
	}
}

#define SETDEFAULT(opt, val) if(!opt){opt = val;}

pu_config_t *pu_config_new_from_file(const char *filename)
{
	pu_config_t *config = pu_config_new();
	if(_pu_config_read_file(filename, config, NULL) != 0) {
		 pu_config_free(config);
		return NULL;
	}

	SETDEFAULT(config->rootdir, strdup("/"));
	SETDEFAULT(config->dbpath, strdup("/var/lib/pacman/"));
	SETDEFAULT(config->logfile, strdup("/var/log/pacman.log"));
	SETDEFAULT(config->cachedirs,
			alpm_list_add(NULL, strdup("/var/cache/pacman/pkg")));

	if(!config->architecture || strcmp(config->architecture, "auto") == 0) {
		struct utsname un;
		uname(&un);
		config->architecture = strdup(un.machine);
	}

	_pu_subst_server_vars(config);

	return config;
}

#undef SETDEFAULT

alpm_handle_t *pu_initialize_handle_from_config(struct pu_config_t *config)
{
	alpm_handle_t *handle = alpm_initialize(config->rootdir, config->dbpath, NULL);

	if(!handle) {
		return NULL;
	}

	alpm_option_set_cachedirs(handle, alpm_list_strdup(config->cachedirs));
	alpm_option_set_noupgrades(handle, alpm_list_strdup(config->noupgrade));
	alpm_option_set_noextracts(handle, alpm_list_strdup(config->noextract));
	alpm_option_set_ignorepkgs(handle, alpm_list_strdup(config->ignorepkgs));
	alpm_option_set_ignoregroups(handle, alpm_list_strdup(config->ignoregroups));

	alpm_option_set_logfile(handle, config->logfile);
	alpm_option_set_gpgdir(handle, config->gpgdir);
	alpm_option_set_usesyslog(handle, config->usesyslog);
	alpm_option_set_arch(handle, config->architecture);

	/*alpm_option_set_default_siglevel(handle, config->siglevel);*/
	/*alpm_option_set_local_file_siglevel(handle, config->localfilesiglevel);*/
	/*alpm_option_set_remote_file_siglevel(handle, config->remotefilesiglevel);*/

	return handle;
}

alpm_db_t *pu_register_syncdb(alpm_handle_t *handle, struct pu_repo_t *repo)
{
	alpm_db_t *db = alpm_register_syncdb(handle, repo->name, ALPM_SIG_USE_DEFAULT);
	if(db) {
		alpm_db_set_servers(db, alpm_list_strdup(repo->servers));
	}
	return db;
}

alpm_list_t *pu_register_syncdbs(alpm_handle_t *handle, alpm_list_t *repos)
{
	alpm_list_t *r, *registered = NULL;
	for(r = repos; r; r = r->next) {
		registered = alpm_list_add(registered, pu_register_syncdb(handle, r->data));
	}
	return registered;
}

const char *pu_alpm_strerror(alpm_handle_t *handle)
{
	alpm_errno_t err = alpm_errno(handle);
	return alpm_strerror(err);
}

#define PU_MAX_REFRESH_MS 200

static long _pu_time_diff(struct timeval *t1, struct timeval *t2)
{
	return (t1->tv_sec - t2->tv_sec) * 1000 + (t1->tv_usec - t2->tv_usec) / 1000;
}

void pu_cb_download(const char *filename, off_t xfered, off_t total)
{
	static struct timeval last_update = {0, 0};
	int percent;

	if(xfered > 0 && xfered < total) {
		struct timeval now;
		gettimeofday(&now, NULL);
		if(_pu_time_diff(&now, &last_update) < PU_MAX_REFRESH_MS) {
			return;
		}
		last_update = now;
	}

	percent = 100 * xfered / total;
	printf("downloading %s (%ld/%ld) %d%%", filename, xfered, total, percent);

	if(xfered == total) {
		putchar('\n');
	} else {
		putchar('\r');
	}

	fflush(stdout);
}

/* vim: set ts=2 sw=2 noet: */