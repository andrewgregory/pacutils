/*
 * Copyright 2013-2016 Andrew Gregory <andrew.gregory.8@gmail.com>
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

#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <strings.h>

#include <pacutils.h>

#include "config-defaults.h"

const char *myname, *myver = BUILDVER;
#define LOG_PREFIX "PACTRANS"

pu_config_t *config = NULL;
alpm_handle_t *handle = NULL;

alpm_loglevel_t log_level = ALPM_LOG_ERROR | ALPM_LOG_WARNING;
alpm_transflag_t trans_flags = 0;

alpm_list_t *spec = NULL, *add = NULL, *rem = NULL, *files = NULL;
alpm_list_t **list = &spec;
alpm_list_t *ignore_pkg = NULL, *ignore_group = NULL, *assume_installed = NULL;
int printonly = 0, noconfirm = 0, sysupgrade = 0, downgrade = 0, dbsync = 0;
int nohooks = 0, isep = '\n';
int resolve_conflict = 0, resolve_replacement = 0;
int install_ignored = 0, remove_corrupted = 0, import_keys = 0;
int default_provider = 0, skip_unresolvable = 0;
const char *dbext = NULL, *sysroot = NULL;

enum longopt_flags {
  FLAG_ADD = 1000,
  FLAG_ASSUME_INSTALLED,
  FLAG_ASDEPS,
  FLAG_ASEXPLICIT,
  FLAG_CACHEDIR,
  FLAG_CASCADE,
  FLAG_CONFIG,
  FLAG_DBEXT,
  FLAG_DBONLY,
  FLAG_DBPATH,
  FLAG_DBSYNC,
  FLAG_DEBUG,
  FLAG_DELETE_CORRUPT,
  FLAG_DLONLY,
  FLAG_DOWNGRADE,
  FLAG_FILE,
  FLAG_HELP,
  FLAG_HOOKDIR,
  FLAG_IGNORE_GROUP,
  FLAG_IGNORE_PKG,
  FLAG_IMPORT_KEYS,
  FLAG_INSTALL_IGNORED,
  FLAG_LOGFILE,
  FLAG_NOBACKUP,
  FLAG_NOCONFIRM,
  FLAG_NODEPS,
  FLAG_NOHOOKS,
  FLAG_NOSCRIPTLET,
  FLAG_NOTIMEOUT,
  FLAG_NULL,
  FLAG_PRINT,
  FLAG_RECURSIVE,
  FLAG_REMOVE,
  FLAG_RESOLVE_CONFLICTS,
  FLAG_RESOLVE_REPLACEMENT,
  FLAG_ROOT,
  FLAG_SKIP_UNRESOLVABLE,
  FLAG_SPEC,
  FLAG_SYSROOT,
  FLAG_SYSUPGRADE,
  FLAG_UNNEEDED,
  FLAG_USE_DEFAULT_PROVIDER,
  FLAG_VERSION,
  FLAG_YOLO,
};

enum replacement_disposition {
  RESOLVE_CONFLICT_PROMPT = 0,
  RESOLVE_CONFLICT_ALL,
  RESOLVE_CONFLICT_NONE,
  RESOLVE_CONFLICT_PROVIDED,
  RESOLVE_CONFLICT_DEPENDS,
  RESOLVE_CONFLICT_PROVIDED_DEPENDS,
};

enum bool_disposition {
  RESOLVE_QUESTION_PROMPT = 0,
  RESOLVE_QUESTION_YES,
  RESOLVE_QUESTION_NO,
};

void fatal(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  pu_ui_verror(fmt, args);
  va_end(args);
  exit(1);
}

void usage(int ret) {
  FILE *stream = (ret ? stderr : stdout);
  alpm_list_t **def = &spec;
  if (myname && strcasecmp(myname, "pacinstall") == 0) {
    def = &add;
  } else if (myname && strcasecmp(myname, "pacremove") == 0) {
    def = &rem;
  }
#define hputf(format, ...) fprintf(stream, format"\n", __VA_ARGS__);
#define hputs(s) fputs(s"\n", stream);
  hputf("%s - install/remove packages", myname);
  hputf("usage:  %s [options] ([<action>] <target>...)...", myname);
  hputf("        %s (--help|--version)", myname);
  hputs("");
  hputs("actions (may be used together):");
  hputf("   --spec             install sync/file specs, remove local specs%s",
      def == &spec ? " (default)" : "");
  hputf("   --install          install packages from sync database%s",
      def == &add ? " (default)" : "");
  hputf("   --remove           remove packages%s",
      def == &rem ? " (default)" : "");
  hputs("   --file             install packages from files");
  hputs("   --sysupgrade       upgrade installed packages");
  hputs("");
  hputs("options:");
  hputs("   --cachedir=<path>  set an alternate cache location");
  hputs("   --config=<path>    set an alternate configuration file");
  hputs("   --dbonly           make the requested changes only to the database");
  hputs("   --dbsync           update sync databases prior to the transaction");
  hputs("   --dbext=<ext>      set an alternate sync database extension");
  hputs("   --dbpath=<path>    set an alternate database location");
  hputs("   --debug            enable extra debugging messages");
  hputs("   --no-timeout       disable low speed timeouts for downloads");
  hputs("   --hookdir=<path>   add additional user hook directory");
  hputs("   --logfile=<path>   set an alternate log file");
  hputs("   --print-only       display transaction information and exit");
  hputs("   --no-confirm       assume default responses to all prompts");
  hputs("   --no-deps          ignore dependency version restrictions");
  hputs("                      (pass twice to ignore dependencies altogether)");
  hputs("   --assume-installed=<package[=version]>");
  hputs("                      behave as if <package> is installed");
  hputs("   --no-hooks         do not run transaction hooks");
  hputs("   --no-scriptlet     do not run package install scripts");
  hputs("   --null=[sep]       parse stdin as <sep> separated values (default NUL)");
  hputs("   --root=<path>      set an alternate installation root");
  hputs("   --sysroot=<path>   set an alternate system root");
  hputs("   --help             display this help information");
  hputs("   --version          display version information");
  hputs("");
  hputs("sysupgrade options:");
  hputs("   --ignore-pkg=<package>");
  hputs("                      ignore upgrades for <package>");
  hputs("   --ignore-group=<group>");
  hputs("                      ignore upgrades for packages in group <group>");
  hputs("");
  hputs("install/file options:");
  hputs("   --as-deps          install packages as dependencies");
  hputs("   --as-explicit      install packages as explicit");
  hputs("   --download-only    download packages without installing");
  hputs("");
  hputs("remove options:");
  hputs("   --cascade          remove packages that depend on removed packages");
  hputs("   --no-backup        do not save configuration file backups");
  hputs("   --recursive        remove unneeded dependencies of removed packages");
  hputs("                      (pass twice to include explicitly installed packages");
  hputs("   --unneeded         only remove packages that are unneeded");
  hputs("");
  hputs("prompt disposition options:");
  hputs("   --resolve-conflicts=<method>");
  hputs("                      method to use for resolving conflicts:");
  hputs("                       prompt           - prompt the user (default)");
  hputs("                       none             - do not remove conflicting packages");
  hputs("                       all              - auto-resolve all conflicts (YOLO)");
  hputs("                       provided         - auto-resolve for provided packages");
  hputs("                       depends          - auto-resolve for dependencies");
  hputs("                       provided-depends - auto-resolve for provided dependencies");
  hputs("   --resolve-replacements=<method>");
  hputs("                      method to use for resolving package replacements:");
  hputs("                       prompt           - prompt the user (default)");
  hputs("                       none             - do not replace any packages");
  hputs("                       all              - auto-confirm all replacements (YOLO)");
  hputs("                       provided         - auto-confirm for provided packages");
  hputs("                       depends          - auto-confirm for dependencies");
  hputs("                       provided-depends - auto-confirm for provided dependencies");
  hputs("   --install-ignored-packages=(prompt|yes|no)");
  hputs("                      enable/disable installation of ignored packages");
  hputs("   --delete-corrupt-files=(prompt|yes|no)");
  hputs("                      enable/disable deletion of corrupt package archives");
  hputs("   --use-first-provider=(prompt|yes|no)");
  hputs("                      enable/disable automatic provider selection");
  hputs("   --skip-unresolvable=(prompt|yes|no)");
  hputs("                      enable/disable skipping packages with missing dependencies");
  hputs("   --import-pgp-keys=(prompt|yes|no)");
  hputs("                      enable/disable automatic importing of missing PGP keys");
  hputs("   --yolo             set all prompt disposition options to all/yes");
  hputs("                      and set --no-confirm");
#undef hputs
#undef hputf
  exit(ret);
}

void parse_replacement_disposition(int *dest, const char *val,
    const char *opt) {
  if (strcmp(val, "prompt") == 0) {
    *dest = RESOLVE_CONFLICT_PROMPT;
  } else if (strcmp(val, "all") == 0) {
    *dest = RESOLVE_CONFLICT_ALL;
  } else if (strcmp(val, "none") == 0) {
    *dest = RESOLVE_CONFLICT_NONE;
  } else if (strcmp(val, "provided-depends") == 0) {
    *dest = RESOLVE_CONFLICT_PROVIDED_DEPENDS;
  } else if (strcmp(val, "depends") == 0) {
    *dest = RESOLVE_CONFLICT_DEPENDS;
  } else if (strcmp(val, "provided") == 0) {
    *dest = RESOLVE_CONFLICT_PROVIDED;
  } else {
    fatal("invalid method passed to %s '%s'", opt, val);
  }
}

void parse_bool_disposition(int *dest, const char *val, const char *opt) {
  if (strcmp(val, "prompt") == 0) {
    *dest = RESOLVE_CONFLICT_PROMPT;
  } else if (strcmp(val, "yes") == 0) {
    *dest = RESOLVE_QUESTION_YES;
  } else if (strcmp(val, "no") == 0) {
    *dest = RESOLVE_QUESTION_NO;
  } else {
    fatal("invalid method passed to %s '%s'", opt, val);
  }
}

pu_config_t *parse_opts(int argc, char **argv) {
  char *config_file = PACMANCONF;
  pu_config_t *config = NULL;
  int c;

  char *short_opts = "-";
  struct option long_opts[] = {
    { "spec", no_argument, NULL, FLAG_SPEC         },
    { "install", no_argument, NULL, FLAG_ADD          },
    { "file", no_argument, NULL, FLAG_FILE         },
    { "remove", no_argument, NULL, FLAG_REMOVE       },

    { "assume-installed", required_argument, NULL, FLAG_ASSUME_INSTALLED     },
    { "cache-dir", required_argument, NULL, FLAG_CACHEDIR     },
    { "cachedir", required_argument, NULL, FLAG_CACHEDIR     },
    { "config", required_argument, NULL, FLAG_CONFIG       },
    { "dbonly", no_argument, NULL, FLAG_DBONLY       },
    { "dbext", required_argument, NULL, FLAG_DBEXT        },
    { "dbpath", required_argument, NULL, FLAG_DBPATH       },
    { "dbsync", no_argument, NULL, FLAG_DBSYNC       },
    { "debug", optional_argument, NULL, FLAG_DEBUG        },
    { "hookdir", required_argument, NULL, FLAG_HOOKDIR      },
    { "logfile", required_argument, NULL, FLAG_LOGFILE      },
    { "print-only", no_argument, NULL, FLAG_PRINT        },
    { "null", optional_argument, NULL, FLAG_NULL         },
    { "no-confirm", no_argument, NULL, FLAG_NOCONFIRM    },
    { "noconfirm", no_argument, NULL, FLAG_NOCONFIRM    },
    { "no-deps", no_argument, NULL, FLAG_NODEPS       },
    { "no-scriptlet", no_argument, NULL, FLAG_NOSCRIPTLET  },
    { "no-hooks", no_argument, NULL, FLAG_NOHOOKS      },
    { "no-timeout", no_argument, NULL, FLAG_NOTIMEOUT    },
    { "root", required_argument, NULL, FLAG_ROOT         },
    { "sysroot", required_argument, NULL, FLAG_SYSROOT      },
    { "sysupgrade", no_argument, NULL, FLAG_SYSUPGRADE   },
    { "downgrade", no_argument, NULL, FLAG_DOWNGRADE    },

    { "help", no_argument, NULL, FLAG_HELP         },
    { "version", no_argument, NULL, FLAG_VERSION      },

    { "ignore-pkg", required_argument, NULL, FLAG_IGNORE_PKG   },
    { "ignore", required_argument, NULL, FLAG_IGNORE_PKG   },
    { "ignore-group", required_argument, NULL, FLAG_IGNORE_GROUP },
    { "ignoregroup", required_argument, NULL, FLAG_IGNORE_GROUP },

    { "as-deps", no_argument, NULL, FLAG_ASDEPS       },
    { "as-explicit", no_argument, NULL, FLAG_ASEXPLICIT   },
    { "download-only", no_argument, NULL, FLAG_DLONLY       },

    { "cascade", no_argument, NULL, FLAG_CASCADE      },
    { "no-backup", no_argument, NULL, FLAG_NOBACKUP     },
    { "recursive", no_argument, NULL, FLAG_RECURSIVE    },
    { "unneeded", no_argument, NULL, FLAG_UNNEEDED     },

    { "resolve-conflicts",        required_argument, NULL, FLAG_RESOLVE_CONFLICTS    },
    { "resolve-replacements",     required_argument, NULL, FLAG_RESOLVE_REPLACEMENT  },
    { "install-ignored-packages", required_argument, NULL, FLAG_INSTALL_IGNORED      },
    { "delete-corrupt-files",     required_argument, NULL, FLAG_DELETE_CORRUPT       },
    { "use-first-provider",       required_argument, NULL, FLAG_USE_DEFAULT_PROVIDER },
    { "skip-unresolvable",        required_argument, NULL, FLAG_SKIP_UNRESOLVABLE    },
    { "import-pgp-keys",          required_argument, NULL, FLAG_IMPORT_KEYS          },
    { "yolo",                     no_argument,       NULL, FLAG_YOLO                 },

    { 0, 0, 0, 0 },
  };

  if ((config = pu_config_new()) == NULL) {
    perror("malloc");
    return NULL;
  }

  /* process remaining command-line options */
  while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
    switch (c) {
      case 0:
        /* already handled */
        break;

      case 1:
        /* non-option arguments */
        *list = alpm_list_add(*list, strdup(optarg));
        break;

      case FLAG_HELP:
        usage(0);
        break;
      case FLAG_VERSION:
        pu_print_version(myname, myver);
        exit(0);
        break;

      /* action selectors */
      case FLAG_ADD:
        list = &add;
        break;
      case FLAG_FILE:
        list = &files;
        break;
      case FLAG_REMOVE:
        list = &rem;
        break;
      case FLAG_SPEC:
        list = &spec;
        break;

      /* general options */
      case FLAG_ASSUME_INSTALLED:
        alpm_list_append_strdup(&assume_installed, optarg);
        break;
      case FLAG_CACHEDIR:
        FREELIST(config->cachedirs);
        config->cachedirs = alpm_list_add(NULL, strdup(optarg));
        break;
      case FLAG_SYSROOT:
        sysroot = optarg;
        break;
      case FLAG_CONFIG:
        config_file = optarg;
        break;
      case FLAG_DBONLY:
        trans_flags |= ALPM_TRANS_FLAG_DBONLY;
        trans_flags |= ALPM_TRANS_FLAG_NOSCRIPTLET;
        break;
      case FLAG_DBEXT:
        dbext = optarg;
        break;
      case FLAG_DBPATH:
        free(config->dbpath);
        config->dbpath = strdup(optarg);
        break;
      case FLAG_DBSYNC:
        dbsync = 1;
        break;
      case FLAG_DEBUG:
        log_level |= ALPM_LOG_DEBUG;
        log_level |= ALPM_LOG_FUNCTION;
        break;
      case FLAG_HOOKDIR:
        config->hookdirs = alpm_list_add(config->hookdirs, strdup(optarg));
        break;
      case FLAG_LOGFILE:
        free(config->logfile);
        config->logfile = strdup(optarg);
        break;
      case FLAG_PRINT:
        printonly = 1;
        trans_flags |= ALPM_TRANS_FLAG_NOLOCK;
        break;
      case FLAG_NOCONFIRM:
        noconfirm = 1;
        break;
      case FLAG_NODEPS:
        if (trans_flags & ALPM_TRANS_FLAG_NODEPVERSION) {
          trans_flags |= ALPM_TRANS_FLAG_NODEPS;
        } else {
          trans_flags |= ALPM_TRANS_FLAG_NODEPVERSION;
        }
        break;
      case FLAG_NULL:
        isep = optarg ? optarg[0] : '\0';
        break;
      case FLAG_NOHOOKS:
        nohooks = 1;
        break;
      case FLAG_NOSCRIPTLET:
        trans_flags |= ALPM_TRANS_FLAG_NOSCRIPTLET;
        break;
      case FLAG_NOTIMEOUT:
        config->disabledownloadtimeout = PU_CONFIG_BOOL_TRUE;
        break;
      case FLAG_ROOT:
        free(config->rootdir);
        config->rootdir = strdup(optarg);
        break;
      case FLAG_SYSUPGRADE:
        sysupgrade = 1;
        break;

      /* sysupgrade options */
      case FLAG_DOWNGRADE:
        downgrade = 1;
        break;
      case FLAG_IGNORE_PKG:
        ignore_pkg = alpm_list_add(ignore_pkg, strdup(optarg));
        break;
      case FLAG_IGNORE_GROUP:
        ignore_group = alpm_list_add(ignore_group, strdup(optarg));
        break;

      /* install options */
      case FLAG_ASDEPS:
        if (trans_flags & ALPM_TRANS_FLAG_ALLEXPLICIT) {
          fatal("--asdeps and --asexplicit may not be used together");
        }
        trans_flags |= ALPM_TRANS_FLAG_ALLDEPS;
        break;
      case FLAG_ASEXPLICIT:
        if (trans_flags & ALPM_TRANS_FLAG_ALLDEPS) {
          fatal("--asdeps and --asexplicit may not be used together");
        }
        trans_flags |= ALPM_TRANS_FLAG_ALLEXPLICIT;
        break;
      case FLAG_DLONLY:
        trans_flags |= ALPM_TRANS_FLAG_DOWNLOADONLY;
        trans_flags |= ALPM_TRANS_FLAG_NOCONFLICTS;
        break;

      /* remove options */
      case FLAG_CASCADE:
        trans_flags |= ALPM_TRANS_FLAG_CASCADE;
        break;
      case FLAG_NOBACKUP:
        trans_flags |= ALPM_TRANS_FLAG_NOSAVE;
        break;
      case FLAG_RECURSIVE:
        if (trans_flags & ALPM_TRANS_FLAG_RECURSE) {
          trans_flags |= ALPM_TRANS_FLAG_RECURSEALL;
        } else {
          trans_flags |= ALPM_TRANS_FLAG_RECURSE;
        }
        break;
      case FLAG_UNNEEDED:
        trans_flags |= ALPM_TRANS_FLAG_UNNEEDED;
        break;

      /* prompt disposition options */
      case FLAG_RESOLVE_CONFLICTS:
        parse_replacement_disposition(&resolve_conflict,
            optarg, "--resolve-conflicts");
        break;
      case FLAG_RESOLVE_REPLACEMENT:
        parse_replacement_disposition(&resolve_replacement,
            optarg, "--resolve-replacements");
        break;
      case FLAG_INSTALL_IGNORED:
        parse_bool_disposition(&install_ignored, optarg,
            "--install-ignored-packages");
        break;
      case FLAG_DELETE_CORRUPT:
        parse_bool_disposition(&remove_corrupted, optarg,
            "--delete-corrupt-files");
        break;
      case FLAG_USE_DEFAULT_PROVIDER:
        parse_bool_disposition(&default_provider, optarg,
            "--use-first-provider");
        break;
      case FLAG_SKIP_UNRESOLVABLE:
        parse_bool_disposition(&skip_unresolvable, optarg,
            "--skip-unresolvable");
        break;
      case FLAG_IMPORT_KEYS:
        parse_bool_disposition(&import_keys, optarg, "--import-pgp-keys");
        break;
      case FLAG_YOLO:
        resolve_conflict = resolve_replacement = RESOLVE_CONFLICT_ALL;
        install_ignored = remove_corrupted = default_provider
                = skip_unresolvable = import_keys = RESOLVE_QUESTION_YES;
        noconfirm = 1;
        break;

      case '?':
      case ':':
        usage(1);
        break;
    }
  }

  if (!pu_ui_config_load_sysroot(config, config_file, sysroot)) {
    fprintf(stderr, "error: could not parse '%s'\n", config_file);
    return NULL;
  }

  return config;
}

void cb_log(void *ctx, alpm_loglevel_t level, const char *fmt, va_list args) {
  (void)ctx;
  if (level & log_level) {
    vprintf(fmt, args);
  }
}

alpm_pkg_t *find_local_pkg(char *pkgspec) {
  if (strchr(pkgspec, '/')) {
    return NULL;
  }

  return alpm_db_get_pkg(alpm_get_localdb(handle), pkgspec);
}

alpm_pkg_t *find_sync_pkg(char *pkgspec) {
  const char *pkgname = pkgspec, *dbname = NULL;
  char *sep = strchr(pkgspec, '/');
  alpm_list_t *d;

  if (sep) {
    dbname = pkgspec;
    pkgname = sep + 1;
    *sep = '\0';
  }

  for (d = alpm_get_syncdbs(handle); d; d = d->next) {
    if (dbname && strcmp(alpm_db_get_name(d->data), dbname) != 0) {
      continue;
    }

    alpm_pkg_t *pkg = alpm_db_get_pkg(d->data, pkgname);
    if (pkg) {
      return pkg;
    }
  }
  return NULL;
}

int load_pkg_files(void) {
  alpm_list_t *i, *remote = NULL, *remote_paths = NULL;
  int ret = 0;
  alpm_siglevel_t slr = alpm_option_get_remote_file_siglevel(handle);
  alpm_siglevel_t sll = alpm_option_get_local_file_siglevel(handle);

  for (i = files; i; i = i->next) {
    if (strstr(i->data, "://") && !alpm_list_append(&remote, i->data)) {
      alpm_list_free(remote);
      return 1;
    }
  }
  if (remote) {
    if ( alpm_fetch_pkgurl(handle, remote, &remote_paths) != 0
        || alpm_list_count(remote) != alpm_list_count(remote_paths)) {
      pu_ui_error("unable to download remote packages");
      alpm_list_free(remote);
      alpm_list_free(remote_paths);
      return 1;
    }
  }

  for (i = files; i; i = i->next) {
    alpm_siglevel_t sl = sll;
    alpm_pkg_t *pkg;

    if (strstr(i->data, "://")) {
      char *path = _pu_list_shift(&remote_paths);
      free(i->data);
      i->data = path;
      sl = slr;
    }

    if ( alpm_pkg_load(handle, i->data, 1, sl, &pkg) != 0) {
      fprintf(stderr, "error: could not load '%s' (%s)\n",
          (char *) i->data, alpm_strerror(alpm_errno(handle)));
      ret++;
    } else {
      add = alpm_list_add(add, pkg);
    }
  }

  return ret;
}

void free_fileconflict(alpm_fileconflict_t *conflict) {
  free(conflict->file);
  free(conflict->target);
  free(conflict->ctarget);
  free(conflict);
}

void print_fileconflict(alpm_fileconflict_t *conflict) {
  switch (conflict->type) {
    case ALPM_FILECONFLICT_TARGET:
      fprintf(stderr, "file conflict: '%s' exists in '%s' and '%s'\n",
          conflict->file, conflict->target, conflict->ctarget);
      break;
    case ALPM_FILECONFLICT_FILESYSTEM:
      if (conflict->ctarget && *conflict->ctarget) {
        fprintf(stderr, "file conflict: '%s' exists in '%s' and filesystem (%s)\n",
            conflict->file, conflict->target, conflict->ctarget);
      } else {
        fprintf(stderr, "file conflict: '%s' exists in '%s' and filesystem\n",
            conflict->file, conflict->target);
      }
      break;
    default:
      fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
      break;
  }
}

int pkg_provides(alpm_pkg_t *pkg1, alpm_pkg_t *pkg2) {
  const char *depname = alpm_pkg_get_name(pkg2);
  alpm_list_t *i;
  for (i = alpm_pkg_get_provides(pkg1); i; i = i->next) {
    alpm_depend_t *d = i->data;
    if (strcmp(d->name, depname) == 0) { return 1; }
  }
  return 0;
}

int should_remove_conflict(enum replacement_disposition d,
    alpm_pkg_t *newpkg, alpm_pkg_t *oldpkg) {
  switch (d) {
    case RESOLVE_CONFLICT_ALL:
      return 1; /* YOLO */
    case RESOLVE_CONFLICT_PROVIDED:
      return pkg_provides(newpkg, oldpkg);
    case RESOLVE_CONFLICT_DEPENDS:
      return alpm_pkg_get_reason(oldpkg) == ALPM_PKG_REASON_DEPEND;
    case RESOLVE_CONFLICT_PROVIDED_DEPENDS:
      return alpm_pkg_get_reason(oldpkg) == ALPM_PKG_REASON_DEPEND
          && pkg_provides(newpkg, oldpkg);
    case RESOLVE_CONFLICT_NONE:
    case RESOLVE_CONFLICT_PROMPT: /* shouldn't happen; bail out */
      return 0;
  }
  return 0; /* shouldn't happen; bail out */
}

void print_q_resolution(alpm_question_t *question) {
  switch (question->type) {
    case ALPM_QUESTION_CONFLICT_PKG: {
      alpm_question_conflict_t *q = (alpm_question_conflict_t *) question;
      alpm_conflict_t *c = q->conflict;
      alpm_list_t *localpkgs = alpm_db_get_pkgcache(alpm_get_localdb(handle));
      alpm_pkg_t *newpkg = alpm_pkg_find(alpm_trans_get_add(handle), alpm_pkg_get_name(c->package1));
      alpm_pkg_t *oldpkg = alpm_pkg_find(localpkgs, alpm_pkg_get_name(c->package2));

      if (q->remove) {
        pu_ui_notice("uninstalling package '%s-%s' due to conflict with '%s-%s'",
            alpm_pkg_get_name(oldpkg), alpm_pkg_get_version(oldpkg),
            alpm_pkg_get_name(newpkg), alpm_pkg_get_version(newpkg));
      }
    }
    break;
    case ALPM_QUESTION_REPLACE_PKG: {
      alpm_question_replace_t *q = (alpm_question_replace_t *) question;
      if (q->replace) {
        pu_ui_notice("replacing package '%s-%s' with '%s-%s'",
            alpm_pkg_get_name(q->oldpkg), alpm_pkg_get_version(q->oldpkg),
            alpm_pkg_get_name(q->newpkg), alpm_pkg_get_version(q->newpkg));
      }
    }
    break;
    case ALPM_QUESTION_INSTALL_IGNOREPKG: {
      alpm_question_install_ignorepkg_t *q = &question->install_ignorepkg;
      if (q->install) {
        pu_ui_notice("installing ignored package '%s-%s'",
            alpm_pkg_get_name(q->pkg), alpm_pkg_get_version(q->pkg));
      } else {
        pu_ui_notice("ignoring package '%s-%s'",
            alpm_pkg_get_name(q->pkg), alpm_pkg_get_version(q->pkg));
      }
    }
    break;
    case ALPM_QUESTION_REMOVE_PKGS: {
      alpm_question_remove_pkgs_t *q = &question->remove_pkgs;
      if (q->skip) {
        alpm_list_t *i;
        for (i = q->packages; i; i = i->next) {
          pu_ui_notice("skipping package '%s-%s' due to unresolvable dependencies",
              alpm_pkg_get_name(i->data), alpm_pkg_get_version(i->data));
        }
      }
    }
    break;
    case ALPM_QUESTION_CORRUPTED_PKG: {
      alpm_question_corrupted_t *q = &question->corrupted;
      if (q->remove) {
        pu_ui_notice("deleting corrupted file '%s' (%s)",
            q->filepath, alpm_strerror(q->reason));
      }
    }
    break;
    case ALPM_QUESTION_IMPORT_KEY: {
      alpm_question_import_key_t *q = &question->import_key;
      if (q->import) {
        pu_ui_notice("Import PGP key %s, '%s'", q->fingerprint, q->uid);
      }
    }
    break;
    case ALPM_QUESTION_SELECT_PROVIDER: {
      alpm_question_select_provider_t *q = &question->select_provider;
      char *depstr = alpm_dep_compute_string(q->depend);
      int idx = q->use_index;
      alpm_list_t *i;
      if (idx >= 0 && (i = alpm_list_nth(q->providers, idx))) {
        pu_ui_notice("selecting package '%s-%s' as provider for dependency '%s'",
            alpm_pkg_get_name(i->data), alpm_pkg_get_version(i->data),
            depstr ? depstr : q->depend->name);
      } else if (idx != -1) {
        /* somebody really messed up to get here */
        pu_ui_error("invalid index (%d) selected for dependency '%s'",
            idx, depstr ? depstr : q->depend->name);
      }
      free(depstr);
    }
    break;
  }
}

int set_bool_response(alpm_question_t *q, enum bool_disposition d) {
  switch (d) {
    case RESOLVE_QUESTION_PROMPT:
      return 0;
    case RESOLVE_QUESTION_YES:
      q->any.answer = 1;
      return 1; /* indicate a response was set */
    case RESOLVE_QUESTION_NO:
      q->any.answer = 0;
      return 1; /* indicate a response was set */
  }
  return 0; /* oh noes, something went wrong */
}

void cb_question(void *ctx, alpm_question_t *question) {
  int autoset = 0;
  (void)ctx;

  switch (question->type) {
    case ALPM_QUESTION_INSTALL_IGNOREPKG:
      autoset = set_bool_response(question, install_ignored);
      break;
    case ALPM_QUESTION_REMOVE_PKGS:
      autoset = set_bool_response(question, skip_unresolvable);
      break;
    case ALPM_QUESTION_CORRUPTED_PKG:
      autoset = set_bool_response(question, remove_corrupted);
      break;
    case ALPM_QUESTION_IMPORT_KEY:
      autoset = set_bool_response(question, import_keys);
      break;
    case ALPM_QUESTION_CONFLICT_PKG:
      if ((autoset = (resolve_conflict != RESOLVE_CONFLICT_PROMPT))) {
        alpm_question_conflict_t *q = (alpm_question_conflict_t *) question;
        alpm_conflict_t *c = q->conflict;
        alpm_list_t *localpkgs = alpm_db_get_pkgcache(alpm_get_localdb(handle));
        alpm_pkg_t *newpkg = alpm_pkg_find(alpm_trans_get_add(handle), alpm_pkg_get_name(c->package1));
        alpm_pkg_t *oldpkg = alpm_pkg_find(localpkgs, alpm_pkg_get_name(c->package2));

        q->remove = should_remove_conflict(resolve_conflict, newpkg, oldpkg);
      }
      break;
    case ALPM_QUESTION_REPLACE_PKG:
      if ((autoset = (resolve_replacement != RESOLVE_CONFLICT_PROMPT))) {
        alpm_question_replace_t *q = (alpm_question_replace_t *) question;
        q->replace = should_remove_conflict(
                resolve_replacement, q->newpkg, q->oldpkg);
      }
      break;
    case ALPM_QUESTION_SELECT_PROVIDER:
      if ((autoset = (default_provider != RESOLVE_QUESTION_PROMPT))) {
        question->any.answer
          = (default_provider == RESOLVE_QUESTION_YES ? 0 : -1);
      }
      break;
  }

  if (autoset || noconfirm) {
    print_q_resolution(question);
  } else {
    pu_ui_cb_question(NULL, question);
  }
}

int main(int argc, char **argv) {
  alpm_list_t *i, *err_data = NULL;
  int ret = 0;
  int have_stdin = !isatty(fileno(stdin)) && errno != EBADF;

  myname = pu_basename(argv[0]);
  if (strcasecmp(myname, "pacinstall") == 0) {
    list = &add;
  } else if (strcasecmp(myname, "pacremove") == 0) {
    list = &rem;
  }

  if (!(config = parse_opts(argc, argv))) {
    goto cleanup;
  }

  if (have_stdin) {
    char *buf = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getdelim(&buf, &len, isep, stdin)) != -1) {
      if (buf[read - 1] == isep) { buf[read - 1] = '\0'; }
      spec = alpm_list_add(spec, strdup(buf));
    }
    free(buf);
  }

  if (!spec && !add && !rem && !files && !sysupgrade) {
    fprintf(stderr, "error: no targets provided.\n");
    ret = 1;
    goto cleanup;
  }

  if (!(handle = pu_initialize_handle_from_config(config))) {
    fprintf(stderr, "error: failed to initialize alpm.\n");
    ret = 1;
    goto cleanup;
  }

  if (dbext && alpm_option_set_dbext(handle, dbext) != 0) {
    fprintf(stderr, "error: unable to set database file extension (%s)\n",
        alpm_strerror(alpm_errno(handle)));
    ret = 1;
    goto cleanup;
  }

  if (nohooks) {
    alpm_option_set_hookdirs(handle, NULL);
  }

  alpm_option_set_questioncb(handle, cb_question, NULL);
  alpm_option_set_progresscb(handle, pu_ui_cb_progress, NULL);
  alpm_option_set_eventcb(handle, pu_ui_cb_event, NULL);
  alpm_option_set_dlcb(handle, pu_ui_cb_download, NULL);
  alpm_option_set_logcb(handle, cb_log, NULL);

  for (i = assume_installed; i; i = i->next) {
    alpm_depend_t *d = alpm_dep_from_string(i->data);
    if(!d) {
      pu_ui_error("unable to parse dependency string '%s'", i->data);
      ret = 1;
      goto cleanup;
    }
    alpm_option_add_assumeinstalled(handle, d);
    alpm_dep_free(d);
  }
  for (i = ignore_pkg; i; i = i->next) {
    alpm_option_add_ignorepkg(handle, i->data);
  }
  for (i = ignore_group; i; i = i->next) {
    alpm_option_add_ignoregroup(handle, i->data);
  }

  if (!pu_register_syncdbs(handle, config->repos)) {
    fprintf(stderr, "error: no valid sync dbs configured.\n");
    ret = 1;
    goto cleanup;
  }

  if (dbsync) {
    alpm_db_update(handle, alpm_get_syncdbs(handle), 0);
  }

  for (i = add; i; i = i->next) {
    char *pkgspec = i->data;
    alpm_pkg_t *p = find_sync_pkg(pkgspec);
    if (p) {
      i->data = p;
    } else {
      fprintf(stderr, "error: could not locate package '%s'\n", pkgspec);
      i->data = NULL;
      ret = 1;
    }
    free(pkgspec);
  }

  for (i = rem; i; i = i->next) {
    char *pkgspec = i->data;
    alpm_pkg_t *p = find_local_pkg(pkgspec);
    if (p) {
      i->data = p;
    } else {
      fprintf(stderr, "error: could not locate package '%s'\n", pkgspec);
      i->data = NULL;
      ret = 1;
    }
    free(pkgspec);
  }

  for (i = spec; i; i = i->next) {
    char *pkgspec = i->data;
    alpm_pkg_t *p = pu_find_pkgspec(handle, pkgspec);
    if (p) {
      switch (alpm_pkg_get_origin(p)) {
        case ALPM_PKG_FROM_SYNCDB:
        case ALPM_PKG_FROM_FILE:
          add = alpm_list_add(add, p);
          break;
        case ALPM_PKG_FROM_LOCALDB:
          rem = alpm_list_add(rem, p);
          break;
      }
    } else {
      fprintf(stderr, "error: could not locate package '%s'\n", pkgspec);
      ret = 1;
    }
    free(pkgspec);
  }

  ret += load_pkg_files();

  if (ret) {
    goto cleanup;
  }

  /**************************************
   * begin transaction
   **************************************/

  if (alpm_trans_init(handle, trans_flags) != 0) {
    fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
    ret = 1;
    goto cleanup;
  }

  for (i = add; i; i = i->next) {
    if (alpm_add_pkg(handle, i->data) != 0) {
      fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
      ret = 1;
      goto transcleanup;
    }
  }

  for (i = rem; i; i = i->next) {
    if (alpm_remove_pkg(handle, i->data) != 0) {
      fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
      ret = 1;
      goto transcleanup;
    }
  }

  if (sysupgrade && alpm_sync_sysupgrade(handle, downgrade) != 0) {
    fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
    ret = 1;
    goto transcleanup;
  }

  if (alpm_trans_prepare(handle, &err_data) != 0) {
    alpm_errno_t err = alpm_errno(handle);
    switch (err) {
      case ALPM_ERR_PKG_INVALID_ARCH:
        for (i = err_data; i; i = alpm_list_next(i)) {
          char *pkgname = i->data;
          fprintf(stderr, "error: invalid architecture (%s)\n", pkgname);
          free(pkgname);
        }
        break;
      case ALPM_ERR_UNSATISFIED_DEPS:
        for (i = err_data; i; i = alpm_list_next(i)) {
          alpm_depmissing_t *dep = i->data;
          char *depstr = alpm_dep_compute_string(dep->depend);
          fprintf(stderr, "error: missing dependency '%s' for package '%s'\n",
              depstr, dep->target);
          free(depstr);
          alpm_depmissing_free(dep);
        }
        break;
      case ALPM_ERR_CONFLICTING_DEPS:
        for (i = err_data; i; i = alpm_list_next(i)) {
          alpm_conflict_t *conflict = i->data;
          fprintf(stderr, "error: package conflict (%s %s)\n",
              alpm_pkg_get_name(conflict->package1), alpm_pkg_get_name(conflict->package2));
          alpm_conflict_free(conflict);
        }
        break;
      default:
        fprintf(stderr, "error: %s\n", alpm_strerror(alpm_errno(handle)));
        break;
    }
    ret = 1;
    goto transcleanup;
  }

  if (!alpm_trans_get_add(handle) && !alpm_trans_get_remove(handle)) {
    fputs("nothing to do\n", stdout);
    goto transcleanup;
  }

  pu_ui_display_transaction(handle);

  if (printonly || (!noconfirm
          && !pu_ui_confirm(1, "Proceed with transaction?")) ) {
    goto transcleanup;
  }

  pu_log_command(handle, LOG_PREFIX, argc, argv);

  if (alpm_trans_commit(handle, &err_data) != 0) {
    switch (alpm_errno(handle)) {
      case ALPM_ERR_PKG_INVALID:
      case ALPM_ERR_PKG_INVALID_CHECKSUM:
      case ALPM_ERR_PKG_INVALID_SIG:
        for (i = err_data; i; i = i->next) {
          char *path = i->data;
          fprintf(stderr, "%s is invalid or corrupted\n", path);
          free(path);
        }
        break;

      case ALPM_ERR_FILE_CONFLICTS:
        for (i = err_data; i; i = i->next) {
          print_fileconflict(i->data);
          free_fileconflict(i->data);
        }
        break;

      default:
        /* FIXME: possible memory leak */
        fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
        break;
    }

    alpm_list_free(err_data);
    err_data = NULL;
    ret = 1;
    goto transcleanup;
  }

transcleanup:
  if (alpm_trans_release(handle) != 0) {
    fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
    ret = 1;
  }

cleanup:
  alpm_list_free(err_data);
  alpm_list_free(add);
  alpm_list_free(rem);
  FREELIST(files);
  FREELIST(ignore_pkg);
  FREELIST(ignore_group);
  alpm_release(handle);
  pu_config_free(config);

  return ret;
}

/* vim: set ts=2 sw=2 noet: */
