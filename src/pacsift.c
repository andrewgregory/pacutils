#define _GNU_SOURCE /* strcasestr */

#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <regex.h>
#include <math.h>

#include <pacutils.h>

#ifndef FILESDBEXT
#define FILESDBEXT ".files"
#endif

const char *myname = "pacsift", *myver = "0.1";

pu_config_t *config = NULL;
alpm_handle_t *handle = NULL;
alpm_loglevel_t log_level = ALPM_LOG_ERROR | ALPM_LOG_WARNING;

int srch_cache = 0, srch_local = 0, srch_sync = 0;
int invert = 0, re = 0, exact = 0, or = 0;
int osep = '\n', isep = '\n';
char *dbext = NULL;
alpm_list_t *search_dbs = NULL;
alpm_list_t *repo = NULL, *name = NULL, *description = NULL, *packager = NULL;
alpm_list_t *base = NULL, *arch = NULL, *url = NULL;
alpm_list_t *group = NULL, *license = NULL;
alpm_list_t *ownsfile = NULL;
alpm_list_t *requiredby = NULL;
alpm_list_t *provides = NULL, *depends = NULL, *conflicts = NULL, *replaces = NULL;
alpm_list_t *satisfies = NULL;
alpm_list_t *isize = NULL, *size = NULL, *dsize = NULL;
alpm_list_t *builddate = NULL, *installdate = NULL;

typedef off_t (size_accessor) (alpm_pkg_t* pkg);
typedef alpm_time_t (date_accessor) (alpm_pkg_t* pkg);
typedef const char* (str_accessor) (alpm_pkg_t* pkg);
typedef alpm_list_t* (strlist_accessor) (alpm_pkg_t* pkg);
typedef alpm_list_t* (deplist_accessor) (alpm_pkg_t* pkg);

enum longopt_flags {
	FLAG_CONFIG = 1000,
	FLAG_DBEXT,
	FLAG_DBPATH,
	FLAG_DEBUG,
	FLAG_HELP,
	FLAG_NULL,
	FLAG_ROOT,
	FLAG_VERSION,

	FLAG_ARCH,
	FLAG_BASE,
	FLAG_BDATE,
	FLAG_CACHE,
	FLAG_NAME,
	FLAG_DESCRIPTION,
	FLAG_GROUP,
	FLAG_IDATE,
	FLAG_ISIZE,
	FLAG_DSIZE,
	FLAG_SIZE,
	FLAG_LICENSE,
	FLAG_OWNSFILE,
	FLAG_PACKAGER,
	FLAG_PROVIDES,
	FLAG_DEPENDS,
	FLAG_CONFLICTS,
	FLAG_REPLACES,
	FLAG_REPO,
	FLAG_SATISFIES,
	FLAG_URL,
};

enum cmp {
	CMP_EQ,
	CMP_NE,
	CMP_GT,
	CMP_LT,
	CMP_GE,
	CMP_LE,
};

struct size_cmp {
	off_t bytes;
	enum cmp cmp;
};

struct date_cmp {
	alpm_time_t time;
	enum cmp cmp;
};

void cleanup(int ret)
{
	alpm_list_free(search_dbs);
	alpm_release(handle);
	pu_config_free(config);

	FREELIST(repo);
	FREELIST(arch);
	FREELIST(name);
	FREELIST(base);
	FREELIST(description);
	FREELIST(packager);

	FREELIST(group);
	FREELIST(license);

	FREELIST(ownsfile);

	FREELIST(provides);
	FREELIST(satisfies);
	FREELIST(url);
	FREELIST(depends);
	FREELIST(conflicts);
	FREELIST(replaces);

	FREELIST(size);
	FREELIST(isize);
	FREELIST(dsize);

	FREELIST(builddate);
	FREELIST(installdate);

	exit(ret);
}

int parse_size_units(off_t *dest, long double bytes, const char *str)
{
	off_t base = 1024, power = 0;
	size_t len;
	int bits = 0;

	if(str == NULL || *str == '\0' || (len = strlen(str)) > 3) { return 0; }

	if(len == 3) {
		if(str[1] == 'i') { base = 1000; }
		else { return 0; }
		if(str[2] == 'b') { bits = 1; }
		else if(str[2] == 'B') { bits = 0; }
		else { return 0; }
	} else if(len == 2) {
		if(str[1] == 'i') { base = 1000; }
		else if(str[1] == 'b') { bits = 1; }
		else if(str[1] == 'B') { bits = 0; }
		else { return 0; }
	}

	switch(str[0]) {
		case 'B': power = 0; if(len > 1) { return 0; } break;
		case 'K': power = 1; break;
		case 'M': power = 2; break;
		case 'G': power = 3; break;
		case 'T': power = 4; break;
		case 'P': power = 5; break;
		case 'E': power = 6; break;
		case 'Z': power = 7; break;
		case 'Y': power = 8; break;
		default: return 0;
	}

	if(power) {
		off_t new = bytes * pow(base, power);
		if(new < bytes) { return 0; }
		else { *dest = new; }
	} else {
		*dest = bytes;
	}
	if(bits) {
		if(*dest % 8) { *dest /= 8; *dest += 1; }
		else { *dest /= 8; }
	}

	return 1;
}

struct date_cmp *parse_date(const char *str)
{
	struct date_cmp date, *ret;
	const char *c = str;
	char *end;
	size_t len;
	struct tm stm;

	memset(&stm, 0, sizeof(struct tm));
	stm.tm_isdst = -1;

	if(c == NULL || *c == '\0') { return NULL; }
	if((len = strspn(c, "=<>!"))) {
		if     (strncmp("=",  c, len) == 0) { date.cmp = CMP_EQ; }
		else if(strncmp(">",  c, len) == 0) { date.cmp = CMP_GT; }
		else if(strncmp("<",  c, len) == 0) { date.cmp = CMP_LT; }
		else if(strncmp("==", c, len) == 0) { date.cmp = CMP_EQ; }
		else if(strncmp("!=", c, len) == 0) { date.cmp = CMP_NE; }
		else if(strncmp(">=", c, len) == 0) { date.cmp = CMP_GE; }
		else if(strncmp("<=", c, len) == 0) { date.cmp = CMP_LE; }
		else {
			fprintf(stderr, "error: invalid date comparison '%s'\n", str);
			cleanup(1);
		}
		str += len;
	} else {
		date.cmp = CMP_EQ;
	}

	if(strspn(str, "0123456789") == strlen(str)) {
		errno = 0;
		date.time = strtoll(str, &end, 10);
		if(*end || errno) {
			fprintf(stderr, "error: invalid date '%s'\n", str);
			cleanup(1);
		}
	} else if((c = strptime(str, "%Y-%m-%d", &stm)) && *c == '\0') {
		date.time = mktime(&stm);
	} else if((c = strptime(str, "%Y-%m-%d %H:%M", &stm)) && *c == '\0') {
		date.time = mktime(&stm);
	} else if((c = strptime(str, "%Y-%m-%d %H:%M:%S", &stm)) && *c == '\0') {
		date.time = mktime(&stm);
	} else {
			fprintf(stderr, "error: invalid date '%s'\n", str);
			cleanup(1);
	}

	if((ret = malloc(sizeof(struct date_cmp))) == NULL) {
		perror("malloc");
		cleanup(1);
	}

	memcpy(ret, &date, sizeof(struct date_cmp));
	return ret;
}

struct size_cmp *parse_size(const char *str)
{
	struct size_cmp size, *ret;
	const char *c = str;
	char *end;
	size_t len;
	long double bytes;
	if(c == NULL || *c == '\0') { return NULL; }
	if((len = strspn(c, "=<>!"))) {
		if     (strncmp("=",  c, len) == 0) { size.cmp = CMP_EQ; }
		else if(strncmp(">",  c, len) == 0) { size.cmp = CMP_GT; }
		else if(strncmp("<",  c, len) == 0) { size.cmp = CMP_LT; }
		else if(strncmp("==", c, len) == 0) { size.cmp = CMP_EQ; }
		else if(strncmp("!=", c, len) == 0) { size.cmp = CMP_NE; }
		else if(strncmp(">=", c, len) == 0) { size.cmp = CMP_GE; }
		else if(strncmp("<=", c, len) == 0) { size.cmp = CMP_LE; }
		else {
			fprintf(stderr, "error: invalid size comparison '%s'\n", str);
			cleanup(1);
		}
		c += len;
	} else {
		size.cmp = CMP_EQ;
	}

	errno = 0;
	bytes = strtold(c, &end);
	if(errno != 0 || end == c) {
		fprintf(stderr, "error: invalid size comparison '%s'\n", str);
		cleanup(1);
	}

	if(bytes > 0) {
		while(isspace(*end)) { end++; }
		if(*end && !parse_size_units(&size.bytes, bytes, end)) {
			fprintf(stderr, "error: invalid size comparison '%s'\n", str);
			cleanup(1);
		}
	} else if(bytes == 0.0) {
		size.bytes = 0;
	} else {
		fprintf(stderr, "error: invalid size comparison '%s'\n", str);
		cleanup(1);
	}

	if((ret = malloc(sizeof(struct size_cmp))) == NULL) {
		perror("malloc");
		cleanup(1);
	}

	memcpy(ret, &size, sizeof(struct size_cmp));
	return ret;
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

alpm_list_t *filter_filelist(alpm_list_t **pkgs, const char *str,
		const char *root, const size_t rootlen)
{
	alpm_list_t *p, *matches = NULL;
	if(strncmp(str, root, rootlen) == 0) { str += rootlen; }
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

int match_date(struct date_cmp *date, alpm_time_t time)
{
	switch(date->cmp) {
		case CMP_EQ: return time == date->time;
		case CMP_NE: return time != date->time;
		case CMP_GT: return time >  date->time;
		case CMP_GE: return time >= date->time;
		case CMP_LT: return time <  date->time;
		case CMP_LE: return time <= date->time;
		default: return 0;
	}
}

int match_size(struct size_cmp *size, off_t bytes)
{
	switch(size->cmp) {
		case CMP_EQ: return bytes == size->bytes;
		case CMP_NE: return bytes != size->bytes;
		case CMP_GT: return bytes >  size->bytes;
		case CMP_GE: return bytes >= size->bytes;
		case CMP_LT: return bytes <  size->bytes;
		case CMP_LE: return bytes <= size->bytes;
		default: return 0;
	}
}

alpm_list_t *filter_date(alpm_list_t **pkgs, struct date_cmp *date, date_accessor *func)
{
	alpm_list_t *p, *matches = NULL;
	for(p = *pkgs; p; p = p->next) {
		alpm_time_t time = func(p->data);
		if(match_date(date, time)) {
			matches = alpm_list_add(matches, p->data);
		}
	}
	for(p = matches; p; p = p->next) {
		*pkgs = alpm_list_remove(*pkgs, p->data, ptr_cmp, NULL);
	}
	return matches;
}

alpm_list_t *filter_size(alpm_list_t **pkgs, struct size_cmp *size, size_accessor *func)
{
	alpm_list_t *p, *matches = NULL;
	for(p = *pkgs; p; p = p->next) {
		off_t bytes = func(p->data);
		if(match_size(size, bytes)) {
			matches = alpm_list_add(matches, p->data);
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

int depcmp(alpm_depend_t *d, alpm_depend_t *needle)
{
	if(needle->name_hash != d->name_hash || strcmp(needle->name, d->name) != 0) {
		return 1;
	}

	if(!exact && !needle->version) { return 0; }

	if(needle->mod == d->mod
			&& alpm_pkg_vercmp(needle->version, d->version) == 0) {
		return 0;
	}

	return 1;
}

alpm_list_t *filter_satisfies(alpm_list_t **pkgs, const char *depstr)
{
	alpm_list_t *matches = NULL;
	alpm_pkg_t *pkg;
	while((pkg = alpm_find_satisfier(*pkgs, depstr))) {
		*pkgs = alpm_list_remove(*pkgs, pkg, ptr_cmp, NULL);
		matches = alpm_list_add(matches, pkg);
	}
	return matches;
}

alpm_list_t *filter_deplist(alpm_list_t **pkgs, const char *str, deplist_accessor *func)
{
	alpm_list_t *p, *matches = NULL;
	alpm_depend_t *needle = alpm_dep_from_string(str);
	if(needle == NULL) {
		fprintf(stderr, "error: invalid dependency '%s'\n", str);
		cleanup(1);
	}
	for(p = *pkgs; p; p = p->next) {
		alpm_list_t *deps = func(p->data);
		if(alpm_list_find(deps, needle, (alpm_list_fn_cmp) depcmp)) {
			matches = alpm_list_add(matches, p->data);
		}
	}
	for(p = matches; p; p = p->next) {
		*pkgs = alpm_list_remove(*pkgs, p->data, ptr_cmp, NULL);
	}
	alpm_dep_free(needle);
	return matches;
}

alpm_list_t *filter_strlist(alpm_list_t **pkgs, const char *str, strlist_accessor *func)
{
	alpm_list_t *p, *matches = NULL;
	if(re) {
		regex_t preg;
		_regcomp(&preg, str, REG_EXTENDED | REG_ICASE | REG_NOSUB);
		for(p = *pkgs; p; p = p->next) {
			alpm_list_t *h = func(p->data);
			for(; h; h = h->next ) {
				if(regexec(&preg, h->data, 0, NULL, 0) == 0) {
					matches = alpm_list_add(matches, p->data);
					break;
				}
			}
		}
		regfree(&preg);
	} else if (exact) {
		for(p = *pkgs; p; p = p->next) {
			if(alpm_list_find_str(func(p->data), str)) {
				matches = alpm_list_add(matches, p->data);
			}
		}
	} else {
		for(p = *pkgs; p; p = p->next) {
			alpm_list_t *h = func(p->data);
			for(; h; h = h->next) {
				if(strcasestr(h->data, str)) {
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

#define match(list, filter) \
	if(list) { \
		alpm_list_t *lp; \
		for(lp = list; lp; lp = alpm_list_next(lp)) { \
			void *i = lp->data; \
			matches = alpm_list_join(matches, filter); \
		} \
		if(!or) { \
			alpm_list_free(haystack); \
			haystack = matches; \
			matches = NULL; \
		} \
	}

alpm_list_t *filter_pkgs(alpm_handle_t *handle, alpm_list_t *pkgs)
{
	alpm_list_t *matches = NULL, *haystack = alpm_list_copy(pkgs);
	const char *root = alpm_option_get_root(handle);
	const size_t rootlen = strlen(root);

	match(name, filter_str(&haystack, i, alpm_pkg_get_name));
	match(base, filter_str(&haystack, i, alpm_pkg_get_base));
	match(description, filter_str(&haystack, i, alpm_pkg_get_desc));
	match(packager, filter_str(&haystack, i, alpm_pkg_get_packager));
	match(repo, filter_str(&haystack, i, get_dbname));
	match(arch, filter_str(&haystack, i, alpm_pkg_get_arch));
	match(group, filter_strlist(&haystack, i, alpm_pkg_get_groups));
	match(license, filter_strlist(&haystack, i, alpm_pkg_get_licenses));
	match(ownsfile, filter_filelist(&haystack, i, root, rootlen));
	match(url, filter_str(&haystack, i, alpm_pkg_get_url));

	match(isize, filter_size(&haystack, i, alpm_pkg_get_isize));
	match(dsize, filter_size(&haystack, i, alpm_pkg_download_size));
	match(size, filter_size(&haystack, i, alpm_pkg_get_size));

	match(builddate, filter_date(&haystack, i, alpm_pkg_get_builddate));
	match(installdate, filter_date(&haystack, i, alpm_pkg_get_installdate));

	match(provides, filter_deplist(&haystack, i, alpm_pkg_get_provides));
	match(depends, filter_deplist(&haystack, i, alpm_pkg_get_depends));
	match(conflicts, filter_deplist(&haystack, i, alpm_pkg_get_conflicts));
	match(replaces, filter_deplist(&haystack, i, alpm_pkg_get_replaces));

	match(satisfies, filter_satisfies(&haystack, i));

	if(invert) {
		matches = alpm_list_diff(pkgs, haystack, ptr_cmp);
		alpm_list_free(haystack);
		return matches;
	} else {
		return or ? matches : haystack;
	}
}

#undef match

void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
#define hputs(str) fputs(str"\n", stream);
	hputs("pacsift - query packages");
	hputs("usage:  pacsift [options] (<field> <term>)...");
	hputs("        pacsift (--help|--version)");
	hputs("options:");
	hputs("   --config=<path>      set an alternate configuration file");
	hputs("   --dbext=<ext>        set an alternate sync database extension");
	hputs("   --dbpath=<path>      set an alternate database location");
	hputs("   --root=<path>        set an alternate installation root");
	hputs("   --null[=sep]         use <sep> to separate values (default NUL)");
	hputs("   --help               display this help information");
	hputs("   --version            display version information");

	hputs("   --invert             display packages which DO NOT match search criteria");
	hputs("   --or                 OR search terms instead of AND");

	hputs("   --exact              match search terms exactly");
	hputs("   --regex              use regular expressions for matching");
	hputs("                        (does not affect dependency matching)");

	hputs(" Filters:");
	hputs("   Note: filters are unaffected by --invert and --or");
	hputs("   --cache              search packages in cache (EXPERIMENTAL)");
	hputs("   --local              search installed packages");
	hputs("   --sync               search packages in all sync repositories");
	/*hputs("   --depends           limit to packages installed as dependencies");*/
	/*hputs("   --explicit          limit to packages installed explicitly");*/
	/*hputs("   --unrequired        limit to unrequired packages");*/
	/*hputs("   --required          limit to required packages");*/
	/*hputs("   --foreign           limit to packages not in a sync repo");*/
	/*hputs("   --native            limit to packages in a sync repo");*/

	hputs(" Package Fields:");
	hputs("   Note: options specified multiple times will be OR'd");
	hputs("   --architecture=<val> search package architecture");
	hputs("   --repo=<val>         search packages in repo <name>");
	hputs("   --name=<val>         search package names");
	hputs("   --base=<val>         search package bases");
	hputs("   --description=<val>  search package descriptions");
	hputs("   --packager=<val>     search package packager field");
	hputs("   --group=<val>        search package group field");
	hputs("   --owns-file=<val>    search package file lists");
	hputs("   --license=<val>      search package licenses");
	hputs("   --provides=<val>     search package provides");
	hputs("   --depends=<val>      search package dependencies");
	hputs("   --conflicts=<val>    search package conflicts");
	hputs("   --replaces=<val>     search package replaces");
	hputs("   --installed-size=<val>");
	hputs("                        search package installed size");
	hputs("   --download-size=<val>");
	hputs("                        search package download size");
	hputs("   --size=<val>         search package size");
	hputs("   --url=<val>          search package url");
	hputs("   --build-date=<val>   search package build date");
	hputs("   --install-date=<val> search package install date");
	hputs("   --satisfies=<val>    find packages satisfying dependency <val>");
#undef hputs

	cleanup(ret);
}

pu_config_t *parse_opts(int argc, char **argv)
{
	char *config_file = "/etc/pacman.conf";
	pu_config_t *config = NULL;
	int c;

	char *short_opts = "QS";
	struct option long_opts[] = {
		{ "config"        , required_argument , NULL    , FLAG_CONFIG        } ,
		{ "dbext"         , required_argument , NULL    , FLAG_DBEXT         } ,
		{ "dbpath"        , required_argument , NULL    , FLAG_DBPATH        } ,
		{ "debug"         , no_argument       , NULL    , FLAG_DEBUG         } ,
		{ "help"          , no_argument       , NULL    , FLAG_HELP          } ,
		{ "version"       , no_argument       , NULL    , FLAG_VERSION       } ,

		{ "cache"         , no_argument       , NULL    , FLAG_CACHE         } ,
		{ "local"         , no_argument       , NULL    , 'Q'                } ,
		{ "sync"          , no_argument       , NULL    , 'S'                } ,

		{ "invert"        , no_argument       , &invert , 1                  } ,
		{ "regex"         , no_argument       , &re     , 1                  } ,
		{ "exact"         , no_argument       , &exact  , 1                  } ,
		{ "or"            , no_argument       , &or     , 1                  } ,

		{ "null"          , optional_argument , NULL    , FLAG_NULL          } ,

		{ "architecture"  , required_argument , NULL    , FLAG_ARCH          } ,
		{ "repo"          , required_argument , NULL    , FLAG_REPO          } ,
		{ "packager"      , required_argument , NULL    , FLAG_PACKAGER      } ,
		{ "name"          , required_argument , NULL    , FLAG_NAME          } ,
		{ "base"          , required_argument , NULL    , FLAG_BASE          } ,
		{ "description"   , required_argument , NULL    , FLAG_DESCRIPTION   } ,
		{ "owns-file"     , required_argument , NULL    , FLAG_OWNSFILE      } ,
		{ "group"         , required_argument , NULL    , FLAG_GROUP         } ,
		{ "license"       , required_argument , NULL    , FLAG_LICENSE       } ,
		{ "url"           , required_argument , NULL    , FLAG_URL           } ,

		{ "provides"      , required_argument , NULL    , FLAG_PROVIDES      } ,
		{ "depends"       , required_argument , NULL    , FLAG_DEPENDS       } ,
		{ "conflicts"     , required_argument , NULL    , FLAG_CONFLICTS     } ,
		{ "replaces"      , required_argument , NULL    , FLAG_REPLACES      } ,

		{ "satisfies"     , required_argument , NULL    , FLAG_SATISFIES     } ,

		{ "installed-size", required_argument , NULL    , FLAG_ISIZE         } ,
		{ "isize"         , required_argument , NULL    , FLAG_ISIZE         } ,
		{ "download-size" , required_argument , NULL    , FLAG_DSIZE         } ,
		{ "dsize"         , required_argument , NULL    , FLAG_DSIZE         } ,
		{ "size"          , required_argument , NULL    , FLAG_SIZE          } ,

		{ "install-date"  , required_argument , NULL    , FLAG_IDATE         } ,
		{ "build-date"    , required_argument , NULL    , FLAG_BDATE         } ,

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
			case FLAG_NULL:
				osep = optarg ? optarg[0] : '\0';
				isep = osep;
				break;
			case FLAG_VERSION:
				pu_print_version(myname, myver);
				cleanup(0);
				break;

			case 'Q':
				srch_local = 1;
				break;
			case 'S':
				srch_sync = 1;
				break;
			case FLAG_CACHE:
				srch_cache = 1;
				break;

			case FLAG_ARCH:
				arch = alpm_list_add(arch, strdup(optarg));
				break;
			case FLAG_REPO:
				repo = alpm_list_add(repo, strdup(optarg));
				break;
			case FLAG_NAME:
				name = alpm_list_add(name, strdup(optarg));
				break;
			case FLAG_BASE:
				base = alpm_list_add(base, strdup(optarg));
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
			case FLAG_GROUP:
				group = alpm_list_add(group, strdup(optarg));
				break;
			case FLAG_LICENSE:
				license = alpm_list_add(license, strdup(optarg));
				break;
			case FLAG_URL:
				url = alpm_list_add(url, strdup(optarg));
				break;

			case FLAG_ISIZE:
				isize = alpm_list_add(isize, parse_size(optarg));
				break;
			case FLAG_DSIZE:
				dsize = alpm_list_add(dsize, parse_size(optarg));
				break;
			case FLAG_SIZE:
				size = alpm_list_add(size, parse_size(optarg));
				break;

			case FLAG_IDATE:
				installdate = alpm_list_add(installdate, parse_date(optarg));
				break;
			case FLAG_BDATE:
				builddate = alpm_list_add(builddate, parse_date(optarg));
				break;

			case FLAG_PROVIDES:
				provides = alpm_list_add(provides, strdup(optarg));
				break;
			case FLAG_DEPENDS:
				depends = alpm_list_add(depends, strdup(optarg));
				break;
			case FLAG_REPLACES:
				replaces = alpm_list_add(replaces, strdup(optarg));
				break;
			case FLAG_CONFLICTS:
				conflicts = alpm_list_add(conflicts, strdup(optarg));
				break;

			case FLAG_SATISFIES:
				satisfies = alpm_list_add(satisfies, strdup(optarg));
				break;

			case '?':
				usage(1);
				break;
		}
	}

	if(!pu_ui_config_load(config, config_file)) {
		fprintf(stderr, "error: could not parse '%s'\n", config_file);
		return NULL;
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

int main(int argc, char **argv)
{
	alpm_list_t *haystack = NULL, *matches = NULL, *i;
	int ret = 0;

	if(!(config = parse_opts(argc, argv))) {
		goto cleanup;
	}

	if(!(handle = pu_initialize_handle_from_config(config))) {
		fprintf(stderr, "error: failed to initialize alpm.\n");
		ret = 1;
		goto cleanup;
	}

	if(ownsfile && dbext == NULL) {
		dbext = FILESDBEXT;
	}

	if(dbext && alpm_option_set_dbext(handle, dbext) != 0) {
		fprintf(stderr, "error: unable to set database file extension (%s)\n",
				alpm_strerror(alpm_errno(handle)));
		ret = 1;
		goto cleanup;
	}

	if(!pu_register_syncdbs(handle, config->repos)) {
		fprintf(stderr, "error: no valid sync dbs configured.\n");
		ret = 1;
		goto cleanup;
	}

	if(!isatty(fileno(stdin)) && errno != EBADF) {
		char *buf = NULL;
		size_t len = 0;
		ssize_t read;

		if(srch_local || srch_sync || srch_cache) {
			fprintf(stderr, "error: --local, --sync, and --cache cannot be used as filters\n");
			ret = 1;
			goto cleanup;
		}

		while((read = getdelim(&buf, &len, isep, stdin)) != -1) {
			alpm_pkg_t *pkg;
			if(buf[read - 1] == isep) { buf[read - 1] = '\0'; }
			if((pkg = pu_find_pkgspec(handle, buf))) {
				haystack = alpm_list_add(haystack, pkg);
			} else {
				fprintf(stderr, "warning: could not locate pkg '%s'\n", buf);
			}
		}

		free(buf);
	} else {
		alpm_list_t *p, *s;

		if(!srch_local && !srch_sync && !srch_cache) {
			srch_local = 1;
			srch_sync = 1;
		}

		if(srch_local) {
			for(p = alpm_db_get_pkgcache(alpm_get_localdb(handle)); p; p = p->next) {
				haystack = alpm_list_add(haystack, p->data);
			}
		}
		if(srch_sync) {
			for(s = alpm_get_syncdbs(handle); s; s = s->next) {
				for(p = alpm_db_get_pkgcache(s->data); p; p = p->next) {
					haystack = alpm_list_add(haystack, p->data);
				}
			}
		}
		if(srch_cache) {
			for(i = alpm_option_get_cachedirs(handle); i; i = i->next) {
				const char *path = i->data;
				DIR *dir = opendir(path);
				struct dirent entry, *result;
				if(!dir) {
					fprintf(stderr, "warning: could not open cache dir '%s' (%s)\n",
							path, strerror(errno));
					continue;
				}
				while(readdir_r(dir, &entry, &result) == 0 && result != NULL) {
					if(strcmp(".", entry.d_name) == 0 || strcmp("..", entry.d_name) == 0) {
						continue;
					}
					size_t path_len = strlen(path) + strlen(entry.d_name);
					char *filename = malloc(path_len + 1);
					int needfiles = ownsfile ? 1 : 0;
					alpm_pkg_t *pkg = NULL;
					sprintf(filename, "%s%s", path, entry.d_name);
					if(alpm_pkg_load(handle, filename, needfiles, 0, &pkg) == 0) {
						haystack = alpm_list_add(haystack, pkg);
					} else {
						fprintf(stderr, "warning: could not load package '%s' (%s)\n",
								filename, alpm_strerror(alpm_errno(handle)));
					}
					free(filename);
				}
				closedir(dir);
			}
		}
	}

	matches = filter_pkgs(handle, haystack);
	for(i = matches; i; i = i->next) {
		pu_fprint_pkgspec(stdout, i->data);
		fputc(osep, stdout);
	}

cleanup:
	alpm_list_free(matches);
	alpm_list_free_inner(haystack, (alpm_list_fn_free) alpm_pkg_free);
	alpm_list_free(haystack);

	cleanup(ret);

	return 0;
}

/* vim: set ts=2 sw=2 noet: */
