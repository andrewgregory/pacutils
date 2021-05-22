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
#include <inttypes.h>
#include <errno.h>
#include <stdio.h>

#include <pacutils.h>

#include "config-defaults.h"

const char *myname = "pacinfo", *myver = BUILDVER;

enum format {
  FORMAT_SHORT = 1,
  FORMAT_LONG = 2,
};

pu_config_t *config = NULL;
alpm_handle_t *handle = NULL;
alpm_list_t *allpkgs = NULL;

int format = FORMAT_LONG, verbosity = 1, removable_size = 0, raw = 0;
int isep = '\n';
const char *dbext = NULL, *sysroot = NULL;
alpm_loglevel_t log_level = ALPM_LOG_ERROR | ALPM_LOG_WARNING;

enum longopt_flags {
  FLAG_CONFIG = 1000,
  FLAG_DBEXT,
  FLAG_DBPATH,
  FLAG_DEBUG,
  FLAG_HELP,
  FLAG_NOTIMEOUT,
  FLAG_NULL,
  FLAG_REMOVABLE,
  FLAG_ROOT,
  FLAG_SYSROOT,
  FLAG_VERBOSE,
  FLAG_VERSION,
};

void prints(const char *field, const char *val) {
  if (val) { printf(field, val); }
}

void printt(const char *field, alpm_time_t t) {
  char time_buf[50];
  struct tm ltime;
  if (t) {
    if (raw) {
      snprintf(time_buf, 50, "%" PRId64, t);
    } else if (sizeof(time_t) < sizeof(alpm_time_t) && (time_t)t != t) {
      /* time does not fit into a time_t, print it raw */
      snprintf(time_buf, 50, "%" PRId64, t);
    } else {
      time_t tt = (time_t)t;
      strftime(time_buf, 50, "%F %T", localtime_r(&tt, &ltime));
    }
    printf(field, time_buf);
  }
}

void printl(const char *field, alpm_list_t *values) {
  while (values) {
    prints(field, values->data);
    values = values->next;
  }
}

void printr(const char *field, alpm_pkg_t *pkg, alpm_list_t *values,
    int deptype) {
  alpm_list_t *v;
  for (v = values; v; v = v->next) {
    alpm_pkg_t *p = v->data;
    alpm_list_t *deps
      = deptype == 0 ? alpm_pkg_get_depends(p)
          : deptype == 1 ? alpm_pkg_get_optdepends(p)
          : deptype == 2 ? alpm_pkg_get_makedepends(p)
          : deptype == 3 ? alpm_pkg_get_checkdepends(p)
          :                NULL;
    for (alpm_list_t *d = deps; d; d = d->next) {
      alpm_depend_t *dep = d->data;
      if (pu_pkg_satisfies_dep(pkg, dep)) {
        if (dep->desc) {
          char *name = pu_pkgspec(p);
          char *str = pu_asprintf("%s: %s", name, dep->desc);
          printf(field, str);
          free(name);
          free(str);
        } else {
          char *name = pu_pkgspec(p);
          printf(field, name);
          free(name);
        }
      }
    }
  }
}

void printd(const char *field, alpm_list_t *values) {
  while (values) {
    char *s = alpm_dep_compute_string(values->data);
    printf(field, s);
    free(s);
    values = values->next;
  }
}

void printo(const char *field, off_t size) {
  char hrsize[50];
  if (raw) {
    snprintf(hrsize, 50, "%lld", (long long)size);
  } else {
    pu_hr_size(size, hrsize);
  }
  printf(field, hrsize);
}

off_t _pkg_removable_size(alpm_pkg_t *pkg, alpm_list_t *pkgs,
    alpm_list_t **seen) {
  const char *pkgname = alpm_pkg_get_name(pkg);
  if (alpm_list_find_ptr(*seen, pkgname))
  { return 0; }

  off_t size = alpm_pkg_get_isize(pkg);
  *seen = alpm_list_add(*seen, (void *) pkgname);

  alpm_list_t *d, *deps = alpm_pkg_get_depends(pkg);
  for (d = deps; d; d = d->next) {
    char *depstring = alpm_dep_compute_string(d->data);
    alpm_pkg_t *p = alpm_find_satisfier(pkgs, depstring);
    free(depstring);

    if (!p) {
      continue;
    }

    if (alpm_pkg_get_reason(p) == ALPM_PKG_REASON_EXPLICIT) {
      continue;
    }

    if (alpm_list_find_ptr(*seen, p)) {
      continue;
    }

    alpm_list_t *rb = alpm_pkg_compute_requiredby(p);

    if (rb) {
      alpm_list_t *diff = alpm_list_diff(rb, *seen,
              (alpm_list_fn_cmp) strcmp);
      if (diff) {
        alpm_list_free(diff);
        FREELIST(rb);
        continue;
      }
      FREELIST(rb);
    }

    size += _pkg_removable_size(p, pkgs, seen);
  }

  return size;
}

off_t pkg_removable_size(alpm_handle_t *handle, alpm_pkg_t *pkg) {
  alpm_db_t *localdb = alpm_get_localdb(handle);
  alpm_list_t *seen = NULL, *installed = alpm_db_get_pkgcache(localdb);
  alpm_list_free(seen);
  return _pkg_removable_size(pkg, installed, &seen);
}

void usage(int ret) {
  FILE *stream = (ret ? stderr : stdout);
#define hputs(s) fputs(s"\n", stream);
  hputs("pacinfo - display package information");
  hputs("usage:  pacinfo [options] <pkgspec>...");
  hputs("        pacinfo (--help|--version)");
  hputs("");
  hputs("options:");
  hputs("   --cachedir=<path>  set an alternate cache location");
  hputs("   --config=<path>    set an alternate configuration file");
  hputs("   --dbext=<ext>      set an alternate sync database extension");
  hputs("   --dbpath=<path>    set an alternate database location");
  hputs("   --no-timeout       disable low speed timeouts for downloads");
  hputs("   --debug            enable extra debugging messages");
  hputs("   --null[=sep]       parse stdin as <sep> separated values (default NUL)");
  hputs("   --short            display brief package information");
  hputs("   --raw              display raw numeric values");
  hputs("   --root=<path>      set an alternate installation root");
  hputs("   --removable-size   include removable dependencies in size");
  hputs("   --verbose          display additional package information");
  hputs("   --help             display this help information");
  hputs("   --version          display version information");
#undef hputs
  exit(ret);
}


pu_config_t *parse_opts(int argc, char **argv) {
  char *config_file = PACMANCONF;
  pu_config_t *config = NULL;
  int c;

  char *short_opts = "";
  struct option long_opts[] = {
    { "config", required_argument, NULL, FLAG_CONFIG       },
    { "help", no_argument, NULL, FLAG_HELP         },
    { "version", no_argument, NULL, FLAG_VERSION      },

    { "dbext", required_argument, NULL, FLAG_DBEXT        },
    { "dbpath", required_argument, NULL, FLAG_DBPATH       },
    { "debug", no_argument, NULL, FLAG_DEBUG        },
    { "root", required_argument, NULL, FLAG_ROOT         },
    { "sysroot", required_argument, NULL, FLAG_SYSROOT      },
    { "null", optional_argument, NULL, FLAG_NULL         },
    { "no-timeout", no_argument, NULL, FLAG_NOTIMEOUT    },

    { "short", no_argument, &format, FORMAT_SHORT      },
    { "raw", no_argument, &raw, 1                 },

    { "verbose", no_argument, NULL, FLAG_VERBOSE      },

    { "removable-size", no_argument, NULL, FLAG_REMOVABLE    },
    { 0, 0, 0, 0 },
  };

  if ((config = pu_config_new()) == NULL) {
    perror("malloc");
    return NULL;
  }

  while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
    switch (c) {

      case 0:
        /* already handled */
        break;

      case FLAG_CONFIG:
        config_file = optarg;
        break;
      case FLAG_HELP:
        usage(0);
        break;
      case FLAG_VERSION:
        pu_print_version(myname, myver);
        exit(0);
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
      case FLAG_REMOVABLE:
        removable_size = 1;
        break;
      case FLAG_ROOT:
        free(config->rootdir);
        config->rootdir = strdup(optarg);
        break;
      case FLAG_SYSROOT:
        sysroot = optarg;
        break;
      case FLAG_NULL:
        isep = optarg ? optarg[0] : '\0';
        break;
      case FLAG_NOTIMEOUT:
        config->disabledownloadtimeout = PU_CONFIG_BOOL_TRUE;
        break;

      case FLAG_VERBOSE:
        verbosity++;
        break;

      case '?':
        usage(1);
        break;
    }
  }

  if (!pu_ui_config_load_sysroot(config, config_file, sysroot)) {
    fprintf(stderr, "error: could not parse '%s'\n", config_file);
    pu_config_free(config);
    return NULL;
  }

  return config;
}

void print_pkg_info(alpm_pkg_t *pkg) {
  alpm_db_t *db = alpm_pkg_get_db(pkg);
  alpm_db_t *localdb = alpm_get_localdb(handle);
  alpm_pkgfrom_t origin = alpm_pkg_get_origin(pkg);
  alpm_list_t *i = NULL;

  switch (format) {
    case FORMAT_SHORT:
      printf("%s/%s %s", alpm_db_get_name(db), alpm_pkg_get_name(pkg),
          alpm_pkg_get_version(pkg));
      {
        alpm_list_t *g = alpm_pkg_get_groups(pkg);
        if (g) {
          fputs(" (", stdout);
          for (; g; g = g->next) {
            fputs(g->data, stdout);
            if (g->next) {
              fputc(' ', stdout);
            }
          }
          fputc(')', stdout);
        }
      }
      if (db != localdb) {
        alpm_pkg_t *lpkg = alpm_db_get_pkg(localdb, alpm_pkg_get_name(pkg));
        if (lpkg) {
          const char *lver = alpm_pkg_get_version(lpkg);
          if (strcmp(lver, alpm_pkg_get_version(pkg)) == 0) {
            fputs(" [installed]", stdout);
          } else {
            printf(" [installed: %s]", lver);
          }
        }
      }
      prints("\n    %s", alpm_pkg_get_desc(pkg));
      break;
    case FORMAT_LONG:
      prints("Name:           %s\n", alpm_pkg_get_name(pkg));
      prints("Base:           %s\n", alpm_pkg_get_base(pkg));
      prints("Repository:     %s\n", alpm_db_get_name(db));
      prints("File:           %s\n", alpm_pkg_get_filename(pkg));
      prints("Version:        %s\n", alpm_pkg_get_version(pkg));
      prints("Description:    %s\n", alpm_pkg_get_desc(pkg));
      prints("Architecture:   %s\n", alpm_pkg_get_arch(pkg));
      prints("URL:            %s\n", alpm_pkg_get_url(pkg));
      printl("Licenses:       %s\n", alpm_pkg_get_licenses(pkg));
      printl("Groups:         %s\n", alpm_pkg_get_groups(pkg));
      printd("Provides:       %s\n", alpm_pkg_get_provides(pkg));
      printd("Requires:       %s\n", alpm_pkg_get_depends(pkg));
      printd("Optional Deps:  %s\n", alpm_pkg_get_optdepends(pkg));
      printd("Conflicts:      %s\n", alpm_pkg_get_conflicts(pkg));
      printd("Replaces:       %s\n", alpm_pkg_get_replaces(pkg));

      if (verbosity >= 2) {
        pu_pkg_find_requiredby(pkg, allpkgs, &i);
        printr("Required By:    %s\n", pkg, i, 0);
        alpm_list_free(i);
        i = NULL;

        pu_pkg_find_optionalfor(pkg, allpkgs, &i);
        printr("Optional For:   %s\n", pkg, i, 1);
        alpm_list_free(i);
        i = NULL;

        pu_pkg_find_makedepfor(pkg, allpkgs, &i);
        printr("MakeDep For:    %s\n", pkg, i, 2);
        alpm_list_free(i);
        i = NULL;

        pu_pkg_find_checkdepfor(pkg, allpkgs, &i);
        printr("CheckDep For:   %s\n", pkg, i, 3);
        alpm_list_free(i);
        i = NULL;
      }

      printo("Package Size:   %s\n", alpm_pkg_get_size(pkg));
      printo("Download Size:  %s\n", alpm_pkg_download_size(pkg));
      printo("Installed Size: %s\n",
          removable_size
          ? pkg_removable_size(handle, pkg)
          : alpm_pkg_get_isize(pkg));
      prints("Packager:       %s\n", alpm_pkg_get_packager(pkg));
      printt("Build Date:     %s\n", alpm_pkg_get_builddate(pkg));
      printt("Install Date:   %s\n", alpm_pkg_get_installdate(pkg));
      prints("MD5 Sum:        %s\n", alpm_pkg_get_md5sum(pkg));
      prints("SHA-256 Sum:    %s\n", alpm_pkg_get_sha256sum(pkg));
      if (verbosity >= 2) {
        prints("Signature:      %s\n", alpm_pkg_get_base64_sig(pkg));
      }

      if (origin != ALPM_PKG_FROM_SYNCDB) {
        prints("Install Script: %s\n",
            alpm_pkg_has_scriptlet(pkg) ? "Yes" : "No");
      }
      if (origin == ALPM_PKG_FROM_LOCALDB) {
        int v = alpm_pkg_get_validation(pkg);
        alpm_pkgreason_t r = alpm_pkg_get_reason(pkg);
        prints("Install Reason: %s\n",
            r == ALPM_PKG_REASON_EXPLICIT ? "Explicit" : "Dependency");
        if (v == ALPM_PKG_VALIDATION_UNKNOWN) {
          prints("Validated By:   %s\n", "Unknown");
        } else {
#define printv(v, m, l) if(v & m) prints("Validated By:   %s\n", l)
          printv(v, ALPM_PKG_VALIDATION_NONE, "None");
          printv(v, ALPM_PKG_VALIDATION_MD5SUM, "MD5");
          printv(v, ALPM_PKG_VALIDATION_SHA256SUM, "SHA-256");
          printv(v, ALPM_PKG_VALIDATION_SIGNATURE, "Signature");
#undef printv
        }
      }
      break;
  }

  putchar('\n');
}

alpm_list_t *find_pkg(const char *pkgspec) {
  alpm_list_t *i, *pkgs = NULL;
  alpm_pkg_t *pkg = pu_find_pkgspec(handle, pkgspec);

  if (pkg) {
    return alpm_list_add(NULL, pkg);
  }

  if ((pkg = alpm_db_get_pkg(alpm_get_localdb(handle), pkgspec))) {
    pkgs = alpm_list_add(pkgs, pkg);
  }

  for (i = alpm_get_syncdbs(handle); i; i = alpm_list_next(i)) {
    if ((pkg = alpm_db_get_pkg(i->data, pkgspec))) {
      pkgs = alpm_list_add(pkgs, pkg);
    }
  }

  return pkgs;
}

int print_pkgspec_info(const char *pkgspec) {
  alpm_list_t *i, *pkgs = find_pkg(pkgspec);
  if (!pkgs) {
    fprintf(stderr, "Unable to find package '%s'\n", pkgspec);
    return -1;
  }
  for (i = pkgs; i; i = alpm_list_next(i)) {
    print_pkg_info(i->data);
  }
  alpm_list_free(pkgs);
  return 0;
}

void cb_log(void *ctx, alpm_loglevel_t level, const char *fmt, va_list args) {
  (void)ctx;
  if (level & log_level) {
    vprintf(fmt, args);
  }
}

int main(int argc, char **argv) {
  int ret = 0;

  if (!(config = parse_opts(argc, argv))) {
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

  alpm_option_set_dlcb(handle, pu_ui_cb_download, NULL);
  alpm_option_set_logcb(handle, cb_log, NULL);

  pu_register_syncdbs(handle, config->repos);
  allpkgs = alpm_list_copy(alpm_db_get_pkgcache(alpm_get_localdb(handle)));
  for (alpm_list_t *i = alpm_get_syncdbs(handle); i; i = i->next) {
    allpkgs = alpm_list_join(allpkgs,
            alpm_list_copy(alpm_db_get_pkgcache(i->data)));
  }

  for (argv += optind; *argv; ++argv) {
    if (print_pkgspec_info(*argv) != 0) { ret = 1; }
  }

  if (!isatty(fileno(stdin)) && errno != EBADF) {
    char *buf = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getdelim(&buf, &len, isep, stdin)) != -1) {
      if (buf[read - 1] == isep) { buf[read - 1] = '\0'; }
      if (print_pkgspec_info(buf) != 0) { ret = 1; }
    }
    free(buf);
  }

cleanup:
  alpm_list_free(allpkgs);
  alpm_release(handle);
  pu_config_free(config);

  return ret;
}

/* vim: set ts=2 sw=2 noet: */
