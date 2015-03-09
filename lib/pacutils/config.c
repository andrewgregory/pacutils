/*
 * Copyright 2012-2015 Andrew Gregory <andrew.gregory.8@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <string.h>
#include <glob.h>
#include <sys/utsname.h>

#include "../../ext/mini.c/mini.c"

#include "config.h"

struct _pu_config_setting {
  char *name;
  pu_config_option_t type;
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

  c = dest;
  next = va_arg(args, char*);
  while(next) {
    c = stpcpy(c, next);
    next = va_arg(args, char*);
    if(next && sep_len) {
      c = stpcpy(c, sep);
    }
  }
  va_end(args);

  return dest;
}

static char *_pu_strreplace(const char *str,
    const char *target, const char *replacement)
{
  const char *ptr;
  char *newstr, *c;
  size_t found = 0;
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

  if((newstr = malloc(newlen + 1)) == NULL) { return NULL; }
  newstr[0] = '\0';

  /* copy the string with the replacements */
  ptr = str;
  c = newstr;
  while((ptr = strstr(ptr, target))) {
    c = stpncpy(c, str, ptr - str);
    c = stpcpy(c, replacement);
    ptr += tlen;
    str = ptr;
  }
  strcat(newstr, str);

  return newstr;
}

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

pu_config_t *pu_config_new(void)
{
  return calloc(sizeof(pu_config_t), 1);
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

pu_repo_t *pu_repo_new(void)
{
  return calloc(sizeof(pu_repo_t), 1);
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

  if(!mini->eof) {
    /* TODO */
    mini_free(mini);
    return -1;
  }

  mini_free(mini);
  return  0;
}

#undef SETSTROPT
#undef FOREACHVAL

void _pu_subst_server_vars(pu_config_t *config)
{
  alpm_list_t *r;
  for(r = config->repos; r; r = r->next) {
    pu_repo_t *repo = r->data;
    alpm_list_t *s;
    for(s = repo->servers; s; s = s->next) {
      char *url = _pu_strreplace(s->data, "$repo", repo->name);
      free(s->data);
      s->data = _pu_strreplace(url, "$arch", config->architecture);
      free(url);
    }
  }
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

alpm_handle_t *pu_initialize_handle_from_config(pu_config_t *config)
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

alpm_db_t *pu_register_syncdb(alpm_handle_t *handle, pu_repo_t *repo)
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

/* vim: set ts=2 sw=2 et: */
