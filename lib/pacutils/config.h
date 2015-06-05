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

#include <alpm.h>

#ifndef PACUTILS_CONFIG_H
#define PACUTILS_CONFIG_H

typedef enum pu_config_option_t {
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
} pu_config_option_t;

/* config */
typedef enum pu_config_cleanmethod_t {
  PU_CONFIG_CLEANMETHOD_KEEP_INSTALLED = (1 << 0),
  PU_CONFIG_CLEANMETHOD_KEEP_CURRENT   = (1 << 1),
} pu_config_cleanmethod_t;

typedef enum pu_config_bool_t {
  PU_CONFIG_BOOL_UNSET = -1,
  PU_CONFIG_BOOL_FALSE = 0,
  PU_CONFIG_BOOL_TRUE = 1,
} pu_config_bool_t;

typedef struct pu_config_t {
  char *rootdir;
  char *dbpath;
  char *gpgdir;
  char *logfile;
  char *architecture;
  char *xfercommand;

  pu_config_bool_t checkspace;
  pu_config_bool_t color;
  pu_config_bool_t ilovecandy;
  pu_config_bool_t totaldownload;
  pu_config_bool_t usesyslog;
  pu_config_bool_t verbosepkglists;

  float usedelta;

  alpm_siglevel_t siglevel;
  alpm_siglevel_t localfilesiglevel;
  alpm_siglevel_t remotefilesiglevel;

  alpm_siglevel_t siglevel_mask;
  alpm_siglevel_t localfilesiglevel_mask;
  alpm_siglevel_t remotefilesiglevel_mask;

  alpm_list_t *cachedirs;
  alpm_list_t *holdpkgs;
  alpm_list_t *ignoregroups;
  alpm_list_t *ignorepkgs;
  alpm_list_t *noextract;
  alpm_list_t *noupgrade;

  pu_config_cleanmethod_t cleanmethod;

  alpm_list_t *repos;
} pu_config_t;

/* sync repos */
typedef struct pu_repo_t {
  char *name;
  alpm_list_t *servers;
  alpm_db_usage_t usage;
  alpm_siglevel_t siglevel;
  alpm_siglevel_t siglevel_mask;
} pu_repo_t;

typedef enum pu_config_reader_status_t {
  PU_CONFIG_READER_STATUS_OK,
  PU_CONFIG_READER_STATUS_ERROR,
  PU_CONFIG_READER_STATUS_INVALID_VALUE,
  PU_CONFIG_READER_STATUS_UNKNOWN_OPTION,
} pu_config_reader_status_t;

typedef struct pu_config_reader_t {
  int eof, line, error;
  char *file, *section, *key, *value;
  pu_config_t *config;
  pu_repo_t *repo;
  pu_config_reader_status_t status;

  void *_mini;
  struct pu_config_reader_t *_parent;
  alpm_list_t *_includes;
} pu_config_reader_t;

pu_repo_t *pu_repo_new(void);
void pu_repo_free(pu_repo_t *repo);
alpm_db_t *pu_register_syncdb(alpm_handle_t *handle, pu_repo_t *repo);
alpm_list_t *pu_register_syncdbs(alpm_handle_t *handle, alpm_list_t *repos);

pu_config_t *pu_config_new(void);
void pu_config_merge(pu_config_t *dest, pu_config_t *src);
int pu_config_resolve(pu_config_t *config);
void pu_config_free(pu_config_t *config);

alpm_handle_t *pu_initialize_handle_from_config(pu_config_t *config);

pu_config_reader_t *pu_config_reader_new(pu_config_t *config, const char *file);
int pu_config_reader_next(pu_config_reader_t *reader);
void pu_config_reader_free(pu_config_reader_t *reader);

#endif /* PACUTILS_CONFIG_H */

/* vim: set ts=2 sw=2 et: */
