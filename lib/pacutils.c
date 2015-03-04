#include "pacutils.h"

#include <glob.h>
#include <sys/time.h>
#include <sys/utsname.h>

#include "../ext/mini.c/mini.c"

char *pu_version(void)
{
	return "0.1";
}

void pu_print_version(const char *progname, const char *progver)
{
	printf("%s v%s - libalpm v%s - pacutils v%s\n", progname, progver,
			alpm_version(), pu_version());
}

int pu_pathcmp(const char *p1, const char *p2)
{
	while(*p1 && *p1 == *p2) {
		/* skip repeated '/' */
		if(*p1 == '/') {
			while(*(++p1) == '/');
			while(*(++p2) == '/');
		} else {
			p1++;
			p2++;
		}
	}

	/* skip trailing '/' */
	if(*p1 == '\0') {
		while(*p2 == '/') p2++;
	} else if(*p2 == '\0') {
		while(*p1 == '/') p1++;
	}

	return *p1 - *p2;
}

static int _pu_filelist_path_cmp(alpm_file_t *f1, alpm_file_t *f2)
{
	return pu_pathcmp(f1->name, f2->name);
}

alpm_file_t *pu_filelist_contains_path(alpm_filelist_t *files, const char *path)
{
	alpm_file_t needle;

	if(files == NULL) {
		return NULL;
	}

	needle.name = (char*) path;

	return bsearch(&needle, files->files, files->count, sizeof(alpm_file_t),
			(int(*)(const void*, const void*)) _pu_filelist_path_cmp);
}

enum _pu_setting_name {
	PU_CONFIG_OPTION_ROOTDIR,
	PU_CONFIG_OPTION_DBPATH,
	PU_CONFIG_OPTION_GPGDIR,
	PU_CONFIG_OPTION_LOGFILE,
	PU_CONFIG_OPTION_ARCHITECTURE,
	PU_CONFIG_OPTION_XFERCOMMAND,

	PU_CONFIG_OPTION_CLEANMETHOD,
	PU_CONFIG_OPTION_COLOR,
	PU_CONFIG_OPTION_USESYSLOG,
	PU_CONFIG_OPTION_USEDELTA,
	PU_CONFIG_OPTION_TOTALDOWNLOAD,
	PU_CONFIG_OPTION_CHECKSPACE,
	PU_CONFIG_OPTION_VERBOSEPKGLISTS,
	PU_CONFIG_OPTION_ILOVECANDY,

	PU_CONFIG_OPTION_SIGLEVEL,
	PU_CONFIG_OPTION_LOCAL_SIGLEVEL,
	PU_CONFIG_OPTION_REMOTE_SIGLEVEL,

	PU_CONFIG_OPTION_HOLDPKGS,
	PU_CONFIG_OPTION_IGNOREPKGS,
	PU_CONFIG_OPTION_IGNOREGROUPS,
	PU_CONFIG_OPTION_NOUPGRADE,
	PU_CONFIG_OPTION_NOEXTRACT,
	PU_CONFIG_OPTION_REPOS,
	PU_CONFIG_OPTION_CACHEDIRS,

	PU_CONFIG_OPTION_SERVER,

	PU_CONFIG_OPTION_USAGE,

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
	{"Color",           PU_CONFIG_OPTION_COLOR},
	{"UseSyslog",       PU_CONFIG_OPTION_USESYSLOG},
	{"UseDelta",        PU_CONFIG_OPTION_USEDELTA},
	{"TotalDownload",   PU_CONFIG_OPTION_TOTALDOWNLOAD},
	{"CheckSpace",      PU_CONFIG_OPTION_CHECKSPACE},
	{"VerbosePkgLists", PU_CONFIG_OPTION_VERBOSEPKGLISTS},
	{"ILoveCandy",      PU_CONFIG_OPTION_ILOVECANDY},

	{"SigLevel",        PU_CONFIG_OPTION_SIGLEVEL},
	{"LocalFileSigLevel",  PU_CONFIG_OPTION_LOCAL_SIGLEVEL},
	{"RemoteFileSigLevel", PU_CONFIG_OPTION_REMOTE_SIGLEVEL},

	{"HoldPkg",         PU_CONFIG_OPTION_HOLDPKGS},
	{"IgnorePkg",       PU_CONFIG_OPTION_IGNOREPKGS},
	{"IgnoreGroup",     PU_CONFIG_OPTION_IGNOREGROUPS},
	{"NoUpgrade",       PU_CONFIG_OPTION_NOUPGRADE},
	{"NoExtract",       PU_CONFIG_OPTION_NOEXTRACT},
	{"CacheDir",        PU_CONFIG_OPTION_CACHEDIRS},

	{"Usage",           PU_CONFIG_OPTION_USAGE},

	{"Include",         PU_CONFIG_OPTION_INCLUDE},

	{"Server",          PU_CONFIG_OPTION_SERVER},

	{NULL, 0}
};

void _pu_parse_cleanmethod(pu_config_t *config, char *val)
{
	char *v, *ctx;
	for(v = strtok_r(val, " ", &ctx); v; v = strtok_r(NULL, " ", &ctx)) {
		if(strcmp(v, "KeepInstalled") == 0) {
			config->cleanmethod |= PU_CONFIG_CLEANMETHOD_KEEP_INSTALLED;
		} else if(strcmp(v, "KeepCurrent") == 0) {
			config->cleanmethod |= PU_CONFIG_CLEANMETHOD_KEEP_CURRENT;
		} else {
			printf("unknown clean method '%s'\n", v);
		}
	}
}

void _pu_parse_siglevel(char *val,
		alpm_siglevel_t *level, alpm_siglevel_t *mask)
{
	char *v, *ctx;

	for(v = strtok_r(val, " ", &ctx); v; v = strtok_r(NULL, " ", &ctx)) {
		int pkg = 1, db = 1;

		if(strncmp(v, "Package", 7) == 0) {
			v += 7;
			db = 0;
		} else if(strncmp(v, "Database", 8) == 0) {
			v += 8;
			pkg = 0;
		}

#define SET(siglevel) do { *level |= (siglevel); *mask |= (siglevel); } while(0)
#define UNSET(siglevel) do { *level &= ~(siglevel); *mask |= (siglevel); } while(0)
		if(strcmp(v, "Never") == 0) {
			if(pkg) { UNSET(ALPM_SIG_PACKAGE | ALPM_SIG_PACKAGE_OPTIONAL); }
			if(db) { UNSET(ALPM_SIG_DATABASE | ALPM_SIG_DATABASE_OPTIONAL); }
		} else if(strcmp(v, "Optional") == 0) {
			if(pkg) { SET(ALPM_SIG_PACKAGE | ALPM_SIG_PACKAGE_OPTIONAL); }
			if(db) { SET(ALPM_SIG_DATABASE | ALPM_SIG_DATABASE_OPTIONAL); }
		} else if(strcmp(v, "Required") == 0) {
			if(pkg) { SET(ALPM_SIG_PACKAGE); UNSET(ALPM_SIG_PACKAGE_OPTIONAL); }
			if(db) { SET(ALPM_SIG_DATABASE); UNSET(ALPM_SIG_DATABASE_OPTIONAL); }
		} else if(strcmp(v, "TrustedOnly") == 0) {
			if(pkg) { UNSET(ALPM_SIG_PACKAGE_MARGINAL_OK | ALPM_SIG_PACKAGE_UNKNOWN_OK); }
			if(db) { UNSET(ALPM_SIG_DATABASE_MARGINAL_OK | ALPM_SIG_DATABASE_UNKNOWN_OK); }
		} else if(strcmp(v, "TrustAll") == 0) {
			if(pkg) { SET(ALPM_SIG_PACKAGE_MARGINAL_OK | ALPM_SIG_PACKAGE_UNKNOWN_OK); }
			if(db) { SET(ALPM_SIG_DATABASE_MARGINAL_OK | ALPM_SIG_DATABASE_UNKNOWN_OK); }
		} else {
			fprintf(stderr, "Invalid SigLevel value '%s'\n", v);
		}
	}
#undef SET
#undef UNSET

	*level &= ~ALPM_SIG_USE_DEFAULT;
}

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

	for(; *start && isspace((int) *start); start++);
	end = start + strlen(start) - 1;
	for(; end > start && isspace((int) *end); end--);

	*(++end) = '\0';
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

int _pu_config_read_file(const char*, pu_config_t*, pu_repo_t**);

int _pu_config_read_glob(const char *val, pu_config_t *config, pu_repo_t **repo)
{
	glob_t gbuf;
	int gret;
	size_t gindex;

	if(!val || val[0] == '\0') {
		return 0;
	}

	gret = glob(val, GLOB_NOCHECK, NULL, &gbuf);
	switch(gret) {
		case GLOB_NOSPACE:
			fprintf(stderr, "include glob out of space\n");
			break;
		default:
			for(gindex = 0; gindex < gbuf.gl_pathc; gindex++) {
				_pu_config_read_file(gbuf.gl_pathv[gindex], config, repo);
			}
			break;
	}
	globfree(&gbuf);

	return 0;
}

int _pu_config_read_file(const char *filename, pu_config_t *config,
		pu_repo_t **repo)
{
	mini_t *mini = mini_init(filename);
	if(!mini) {
		return -1;
	}

	while(mini_next(mini)) {
		if(!mini->key) {
			if(strcmp(mini->section, "options") == 0) {
				*repo = NULL;
			} else {
				pu_repo_t *r = pu_repo_new();
				r->name = strdup(mini->section);
				r->siglevel = ALPM_SIG_USE_DEFAULT;
				config->repos = alpm_list_add(config->repos, r);
				*repo = r;
			}
		} else {
			char *v, *ctx;
			struct _pu_config_setting *s;

#define FOREACHVAL for(v = strtok_r(mini->value, " ", &ctx); v; v = strtok_r(NULL, " ", &ctx))
#define SETSTROPT(dest, val) if(dest == NULL) { \
	char *dup = strdup(val); \
	if(dup) { \
		free(dest); \
		dest = dup; \
	} else { \
		fprintf(stderr, "Insufficient memory\n"); \
	} \
}

			if(!(s = _pu_config_lookup_setting(mini->key))) {
				printf("unknown option '%s'\n", mini->key);
				continue;
			}

			if(*repo) {
				pu_repo_t *r = *repo;
				switch(s->type) {
					case PU_CONFIG_OPTION_INCLUDE:
						_pu_config_read_glob(mini->value, config, repo);
						break;
					case PU_CONFIG_OPTION_SIGLEVEL:
						_pu_parse_siglevel(mini->value, &(r->siglevel),
								&(r->siglevel_mask));
						break;
					case PU_CONFIG_OPTION_SERVER:
						r->servers = alpm_list_add(r->servers, strdup(mini->value));
						break;
					case PU_CONFIG_OPTION_USAGE:
						FOREACHVAL {
							if(strcmp(v, "Sync") == 0) {
								r->usage |= ALPM_DB_USAGE_SYNC;
							} else if(strcmp(v, "Search") == 0) {
								r->usage |= ALPM_DB_USAGE_SEARCH;
							} else if(strcmp(v, "Install") == 0) {
								r->usage |= ALPM_DB_USAGE_INSTALL;
							} else if(strcmp(v, "Upgrade") == 0) {
								r->usage |= ALPM_DB_USAGE_UPGRADE;
							} else if(strcmp(v, "All") == 0) {
								r->usage |= ALPM_DB_USAGE_ALL;
							} else {
								printf("unknown db usage level '%s'\n", v);
							}
						}
						break;
					default:
						/* TODO */
						break;
				}
			} else {
				switch(s->type) {
					case PU_CONFIG_OPTION_ROOTDIR:
						SETSTROPT(config->rootdir, mini->value);
						break;
					case PU_CONFIG_OPTION_DBPATH:
						SETSTROPT(config->dbpath, mini->value);
						break;
					case PU_CONFIG_OPTION_GPGDIR:
						SETSTROPT(config->gpgdir, mini->value);
						break;
					case PU_CONFIG_OPTION_LOGFILE:
						SETSTROPT(config->logfile, mini->value);
						break;
					case PU_CONFIG_OPTION_ARCHITECTURE:
						SETSTROPT(config->architecture, mini->value);
						break;
					case PU_CONFIG_OPTION_XFERCOMMAND:
						free(config->xfercommand);
						config->xfercommand = strdup(mini->value);
						break;
					case PU_CONFIG_OPTION_CLEANMETHOD:
						_pu_parse_cleanmethod(config, mini->value);
						break;
					case PU_CONFIG_OPTION_COLOR:
						config->color = 1;
						break;
					case PU_CONFIG_OPTION_USESYSLOG:
						config->usesyslog = 1;
						break;
					case PU_CONFIG_OPTION_USEDELTA:
						if(mini->value) {
							char *end;
							float d = strtof(mini->value, &end);
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
					case PU_CONFIG_OPTION_ILOVECANDY:
						config->ilovecandy = 1;
						break;
					case PU_CONFIG_OPTION_SIGLEVEL:
						_pu_parse_siglevel(mini->value, &(config->siglevel),
								&(config->siglevel_mask));
						break;
					case PU_CONFIG_OPTION_LOCAL_SIGLEVEL:
						_pu_parse_siglevel(mini->value, &(config->localfilesiglevel),
								&(config->localfilesiglevel_mask));
						break;
					case PU_CONFIG_OPTION_REMOTE_SIGLEVEL:
						_pu_parse_siglevel(mini->value, &(config->remotefilesiglevel),
								&(config->remotefilesiglevel_mask));
						break;
					case PU_CONFIG_OPTION_HOLDPKGS:
						FOREACHVAL {
							config->holdpkgs = alpm_list_add(config->holdpkgs, strdup(v));
						}
						break;
					case PU_CONFIG_OPTION_IGNOREPKGS:
						FOREACHVAL {
							config->ignorepkgs = alpm_list_add(config->ignorepkgs, strdup(v));
						}
						break;
					case PU_CONFIG_OPTION_IGNOREGROUPS:
						FOREACHVAL {
							config->ignoregroups = alpm_list_add(config->ignoregroups, strdup(v));
						}
						break;
					case PU_CONFIG_OPTION_NOUPGRADE:
						FOREACHVAL {
							config->noupgrade = alpm_list_add(config->noupgrade, strdup(v));
						}
						break;
					case PU_CONFIG_OPTION_NOEXTRACT:
						FOREACHVAL {
							config->noextract = alpm_list_add(config->noextract, strdup(v));
						}
						break;
					case PU_CONFIG_OPTION_CACHEDIRS:
						FOREACHVAL {
							config->cachedirs = alpm_list_add(config->cachedirs, strdup(v));
						}
						break;
					case PU_CONFIG_OPTION_INCLUDE:
						_pu_config_read_glob(mini->value, config, repo);
						break;
					default:
						/* TODO */
						break;
				}
			}
		}
	}

#undef SETSTROPT
#undef FOREACHVAL

	if(!mini->eof) {
		/* TODO */
		mini_free(mini);
		return -1;
	}

	mini_free(mini);
	return  0;
}

void _pu_subst_server_vars(pu_config_t *config)
{
	alpm_list_t *r;
	for(r = config->repos; r; r = r->next) {
		pu_repo_t *repo = r->data;
		alpm_list_t *s;
		for(s = repo->servers; s; s = s->next) {
			char *url = pu_strreplace(s->data, "$repo", repo->name);
			free(s->data);
			s->data = pu_strreplace(url, "$arch", config->architecture);
			free(url);
		}
	}
}

static char *_pu_strjoin(const char *sep, ...)
{
	char *c, *next, *dest;
	size_t tlen = 0, sep_len = (sep && *sep) ? strlen(sep) : 0;
	int argc = 0;
	va_list calc, args;

	va_start(args, sep);

	va_copy(calc, args);
	for(tlen = 0; (c = va_arg(calc, char*)); argc++, tlen += strlen(c) + sep_len);
	tlen -= sep_len;
	va_end(calc);

	if(argc == 0) {
		va_end(args);
		return strdup("");
	} else if((dest = malloc(tlen + 1)) == NULL) {
		va_end(args);
		return NULL;
	}

	dest[0] = '\0';
	next = va_arg(args, char*);
	while(next) {
		c = next;
		next = va_arg(args, char*);
		strcat(dest, c);
		if(next && sep_len) {
			strcat(dest, sep);
		}
	}
	va_end(args);

	return dest;
}

pu_config_t *pu_config_new_from_file(const char *filename)
{
	pu_config_t *config = pu_config_new();
	alpm_list_t *i;
	pu_repo_t *repo;

	/* 0 is a valid siglevel value, so these must be set before parsing */
	config->siglevel = (
			ALPM_SIG_PACKAGE | ALPM_SIG_PACKAGE_OPTIONAL |
			ALPM_SIG_DATABASE | ALPM_SIG_DATABASE_OPTIONAL);
	config->localfilesiglevel = ALPM_SIG_USE_DEFAULT;
	config->remotefilesiglevel = ALPM_SIG_USE_DEFAULT;

	if(_pu_config_read_file(filename, config, &repo) != 0) {
		 pu_config_free(config);
		return NULL;
	}

#define SETDEFAULT(opt, val) if(!opt) { opt = val; }
	if(config->rootdir) {
		SETDEFAULT(config->dbpath,
				_pu_strjoin("/", config->rootdir, "var/lib/pacman/", NULL));
		SETDEFAULT(config->logfile,
				_pu_strjoin("/", config->rootdir, "var/log/pacman.log", NULL));
	} else {
		SETDEFAULT(config->rootdir, strdup("/"));
		SETDEFAULT(config->dbpath, strdup("/var/lib/pacman/"));
		SETDEFAULT(config->logfile, strdup("/var/log/pacman.log"));
	}
	SETDEFAULT(config->gpgdir, strdup("/etc/pacman.d/gnupg/"));
	SETDEFAULT(config->cachedirs,
			alpm_list_add(NULL, strdup("/var/cache/pacman/pkg")));
	SETDEFAULT(config->cleanmethod, PU_CONFIG_CLEANMETHOD_KEEP_INSTALLED);

	if(!config->architecture || strcmp(config->architecture, "auto") == 0) {
		struct utsname un;
		uname(&un);
		free(config->architecture);
		config->architecture = strdup(un.machine);
	}

#define SETSIGLEVEL(l, m) if(m) { l = (l & (m)) | (config->siglevel & ~(m)); }
	SETSIGLEVEL(config->localfilesiglevel, config->localfilesiglevel_mask);
	SETSIGLEVEL(config->remotefilesiglevel, config->remotefilesiglevel_mask);

	for(i = config->repos; i; i = i->next) {
		pu_repo_t *r = i->data;
		SETDEFAULT(r->usage, ALPM_DB_USAGE_ALL);
		SETSIGLEVEL(r->siglevel, r->siglevel_mask);
	}
#undef SETSIGLEVEL
#undef SETDEFAULT

	_pu_subst_server_vars(config);

	return config;
}

alpm_handle_t *pu_initialize_handle_from_config(struct pu_config_t *config)
{
	alpm_handle_t *handle = alpm_initialize(config->rootdir, config->dbpath, NULL);

	if(!handle) {
		return NULL;
	}

	alpm_option_set_cachedirs(handle, config->cachedirs);
	alpm_option_set_noupgrades(handle, alpm_list_strdup(config->noupgrade));
	alpm_option_set_noextracts(handle, alpm_list_strdup(config->noextract));
	alpm_option_set_ignorepkgs(handle, alpm_list_strdup(config->ignorepkgs));
	alpm_option_set_ignoregroups(handle, alpm_list_strdup(config->ignoregroups));

	alpm_option_set_logfile(handle, config->logfile);
	alpm_option_set_gpgdir(handle, config->gpgdir);
	alpm_option_set_usesyslog(handle, config->usesyslog);
	alpm_option_set_arch(handle, config->architecture);

	alpm_option_set_default_siglevel(handle, config->siglevel);
	alpm_option_set_local_file_siglevel(handle, config->localfilesiglevel);
	alpm_option_set_remote_file_siglevel(handle, config->remotefilesiglevel);

	return handle;
}

alpm_db_t *pu_register_syncdb(alpm_handle_t *handle, struct pu_repo_t *repo)
{
	alpm_db_t *db = alpm_register_syncdb(handle, repo->name, repo->siglevel);
	if(db) {
		alpm_db_set_servers(db, alpm_list_strdup(repo->servers));
		alpm_db_set_usage(db, repo->usage);
	}
	return db;
}

alpm_list_t *pu_register_syncdbs(alpm_handle_t *handle, alpm_list_t *repos)
{
	alpm_list_t *r;
	for(r = repos; r; r = r->next) {
		pu_register_syncdb(handle, r->data);
	}
	return alpm_get_syncdbs(handle);
}

#define PU_MAX_REFRESH_MS 200

static long _pu_time_diff(struct timeval *t1, struct timeval *t2)
{
	return (t1->tv_sec - t2->tv_sec) * 1000 + (t1->tv_usec - t2->tv_usec) / 1000;
}

void pu_cb_question(alpm_question_t *question)
{
	switch(question->type) {
		case ALPM_QUESTION_INSTALL_IGNOREPKG:
			{
				alpm_question_install_ignorepkg_t *q = &question->install_ignorepkg;
				q->install = pu_confirm(1,
						"Install ignored package '%s'?", alpm_pkg_get_name(q->pkg));
			}
			break;
		case ALPM_QUESTION_REPLACE_PKG:
			{
				alpm_question_replace_t *q = &question->replace;
				q->replace = pu_confirm(1, "Replace '%s' with '%s'?",
						alpm_pkg_get_name(q->oldpkg), alpm_pkg_get_name(q->newpkg));
			}
			break;
		case ALPM_QUESTION_CONFLICT_PKG:
			{
				alpm_question_conflict_t *q = (alpm_question_conflict_t*) question;
				alpm_conflict_t *c = q->conflict;
				q->remove = pu_confirm(1, "'%s' conflicts with '%s'.  Remove '%s'?",
						c->package1, c->package2, c->package2);
			}
			break;
		case ALPM_QUESTION_REMOVE_PKGS:
		case ALPM_QUESTION_SELECT_PROVIDER:
		case ALPM_QUESTION_CORRUPTED_PKG:
		case ALPM_QUESTION_IMPORT_KEY:
		default:
			break;
	}
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

const char *pu_msg_progress(alpm_progress_t event)
{
	switch(event) {
		case ALPM_PROGRESS_ADD_START:
			return "installing";
		case ALPM_PROGRESS_UPGRADE_START:
			return "upgrading";
		case ALPM_PROGRESS_DOWNGRADE_START:
			return "downgrading";
		case ALPM_PROGRESS_REINSTALL_START:
			return "reinstalling";
		case ALPM_PROGRESS_REMOVE_START:
			return "removing";
		case ALPM_PROGRESS_CONFLICTS_START:
			return "checking for file conflicts";
		case ALPM_PROGRESS_DISKSPACE_START:
			return "checking available disk space";
		case ALPM_PROGRESS_INTEGRITY_START:
			return "checking package integrity";
		case ALPM_PROGRESS_KEYRING_START:
			return "checking keys in keyring";
		case ALPM_PROGRESS_LOAD_START:
			return "loading package files";
		default:
			return "working";
	}
}

void pu_cb_progress(alpm_progress_t event, const char *pkgname, int percent,
		size_t total, size_t current)
{
	const char *opr = pu_msg_progress(event);
	static int percent_last = -1;

	/* don't update if nothing has changed */
	if(percent_last == percent) {
		return;
	}

	if(pkgname && pkgname[0]) {
		printf("%s %s (%zd/%zd) %d%%", opr, pkgname, current, total, percent);
	} else {
		printf("%s (%zd/%zd) %d%%", opr, current, total, percent);
	}

	if(percent == 100) {
		putchar('\n');
	} else {
		putchar('\r');
	}

	fflush(stdout);
	percent_last = percent;
}

alpm_pkg_t *pu_find_pkgspec(alpm_handle_t *handle, const char *pkgspec)
{
	char *c;

	if(strstr(pkgspec, "://")) {
		alpm_pkg_t *pkg;
		alpm_siglevel_t sl
			= strncmp(pkgspec, "file://", 7) == 0
			? alpm_option_get_local_file_siglevel(handle)
			: alpm_option_get_remote_file_siglevel(handle);
		char *path = alpm_fetch_pkgurl(handle, pkgspec);
		int err = alpm_pkg_load(handle, path ? path : pkgspec, 1, sl, &pkg);
		free(path);
		if(!err) {
			return pkg;
		}
	} else if((c = strchr(pkgspec, '/')))  {
		alpm_db_t *db = NULL;
		size_t dblen = c - pkgspec;

		if(dblen == strlen("local") && strncmp(pkgspec, "local", dblen) == 0) {
			db = alpm_get_localdb(handle);
		} else {
			alpm_list_t *i;
			for(i = alpm_get_syncdbs(handle); i; i = i->next) {
				const char *dbname = alpm_db_get_name(i->data);
				if(dblen == strlen(dbname) && strncmp(pkgspec, dbname, dblen) == 0) {
					db = i->data;
					break;
				}
			}
		}

		if(!db) {
			return NULL;
		} else {
			return alpm_db_get_pkg(db, c + 1);
		}
	}

	return NULL;
}

/**
 * @brief print unique identifier for a package
 *
 * @param pkg
 */
void pu_fprint_pkgspec(FILE *stream, alpm_pkg_t *pkg)
{
	const char *c;
	switch(alpm_pkg_get_origin(pkg)) {
		case ALPM_PKG_FROM_FILE:
			c = alpm_pkg_get_filename(pkg);
			if(strstr(c, "://")) {
				fprintf(stream, "%s", alpm_pkg_get_filename(pkg));
			} else {
				c = realpath(c, NULL);
				fprintf(stream, "file://%s", c);
				free((char*) c);
			}
			break;
		case ALPM_PKG_FROM_LOCALDB:
			fprintf(stream, "local/%s", alpm_pkg_get_name(pkg));
			break;
		case ALPM_PKG_FROM_SYNCDB:
			fprintf(stream, "%s/%s",
					alpm_db_get_name(alpm_pkg_get_db(pkg)), alpm_pkg_get_name(pkg));
			break;
		default:
			/* no idea where this package came from, fall back to its name */
			fputs(alpm_pkg_get_name(pkg), stream);
			break;
	}
}

char *pu_hr_size(off_t bytes, char *dest)
{
	static char *suff[] = {"B", "K", "M", "G", "T", "P", "E", NULL};
	float hrsize;
	int s = 0;
	while((bytes >= 1000000 || bytes <= -1000000) && suff[s + 1]) {
		bytes /= 1024;
		++s;
	}
	hrsize = bytes;
	if((hrsize >= 1000 || hrsize <= -1000) && suff[s + 1]) {
		hrsize /= 1024;
		++s;
	}
	sprintf(dest, "%.2f %s", hrsize, suff[s]);
	return dest;
}

void pu_display_transaction(alpm_handle_t *handle)
{
	off_t install = 0, download = 0, delta = 0;
	char size[20];
	alpm_db_t *ldb = alpm_get_localdb(handle);
	alpm_list_t *i;

	for(i = alpm_trans_get_remove(handle); i; i = i->next) {
		alpm_pkg_t *p = i->data;
		printf("removing %s/%s (%s)\n",
				alpm_db_get_name(alpm_pkg_get_db(p)),
				alpm_pkg_get_name(p),
				alpm_pkg_get_version(p));

		install -= alpm_pkg_get_isize(i->data);
		delta -= alpm_pkg_get_isize(i->data);
	}

	for(i = alpm_trans_get_add(handle); i; i = i->next) {
		alpm_pkg_t *p = i->data;
		alpm_pkg_t *lpkg = alpm_db_get_pkg(ldb, alpm_pkg_get_name(p));

		switch(alpm_pkg_get_origin(p)) {
			case ALPM_PKG_FROM_FILE:
				printf("installing %s (%s)",
						alpm_pkg_get_filename(p),
						alpm_pkg_get_name(p));
				break;
			case ALPM_PKG_FROM_SYNCDB:
				printf("installing %s/%s",
						alpm_db_get_name(alpm_pkg_get_db(p)),
						alpm_pkg_get_name(p));
				break;
			case ALPM_PKG_FROM_LOCALDB:
				/* impossible */
				break;
		}

		if(lpkg) {
			printf(" (%s -> %s)\n",
					alpm_pkg_get_version(lpkg), alpm_pkg_get_version(p));
		} else {
			printf(" (%s)\n", alpm_pkg_get_version(p));
		}

		install  += alpm_pkg_get_isize(p);
		download += alpm_pkg_download_size(p);
		delta    += alpm_pkg_get_isize(p);
		if(lpkg) {
			delta  -= alpm_pkg_get_isize(lpkg);
		}
	}

	fputs("\n", stdout);
	printf("Download Size:  %10s\n", pu_hr_size(download, size));
	printf("Installed Size: %10s\n", pu_hr_size(install, size));
	printf("Size Delta:     %10s\n", pu_hr_size(delta, size));
}

int pu_confirm(int def, const char *prompt, ...)
{
	va_list args;
	va_start(args, prompt);
	fputs("\n:: ", stdout);
	vprintf(prompt, args);
	fputs(def ? " [Y/n]" : " [y/N]", stdout);
	va_end(args);
	while(1) {
		int c = getchar();
		if(c != '\n') {
			while(getchar() != '\n');
		}

		switch(c) {
			case '\n':
				return def;
			case 'Y':
			case 'y':
				return 1;
			case 'N':
			case 'n':
				return 0;
		}
	}
}

int pu_log_command(alpm_handle_t *handle, const char *caller, int argc, char **argv)
{
	int i;
	char *cmd;
	size_t cmdlen = strlen("Running");

	for(i = 0; i < argc; ++i) {
		cmdlen += strlen(argv[i]) + 1;
	}

	cmd = malloc(cmdlen + 2);
	if(!cmd) {
		return -1;
	}

	strcpy(cmd, "Running");
	for(i = 0; i < argc; ++i) {
		strcat(cmd, " ");
		strcat(cmd, argv[i]);
	}
	strcat(cmd, "\n");

	alpm_logaction(handle, caller, cmd);

	free(cmd);

	return 0;
}

char *pu_basename(char *path)
{
	char *c;

	if(!path) {
		return NULL;
	}

	for(c = path + strlen(path); c > path && *(c - 1) != '/'; --c) {
		/* empty loop */
	}

	return c;
}

/* vim: set ts=2 sw=2 noet: */
