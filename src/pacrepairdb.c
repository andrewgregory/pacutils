/*
 * Copyright 2016 Andrew Gregory <andrew.gregory.8@gmail.com>
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>

#include <pacutils.h>

#include "config-defaults.h"

const char *myname = "pacrepairdb", *myver = BUILDVER;
const char *log_prefix = "PACREPAIRDB";

enum longopt_flags {
  FLAG_CACHEDIR = 1000,
  FLAG_CONFIG,
  FLAG_DBPATH,
  FLAG_DBONLY,
  FLAG_DEBUG,
  FLAG_HELP,
  FLAG_HOOKDIR,
  FLAG_LOGFILE,
  FLAG_NOCONFIRM,
  FLAG_NOSCRIPTLET,
  FLAG_NOHOOKS,
  FLAG_NOTIMEOUT,
  FLAG_PRINT,
  FLAG_ROOT,
  FLAG_SYSROOT,
  FLAG_VERBOSE,
  FLAG_VERSION,
};

pu_config_t *config = NULL;
alpm_handle_t *handle = NULL;
int noconfirm = 0, nohooks = 0, printonly = 0, verbose = 0;
int log_level = ALPM_LOG_ERROR | ALPM_LOG_WARNING;
int trans_flags = ALPM_TRANS_FLAG_NODEPS | ALPM_TRANS_FLAG_NOCONFLICTS;
char *sysroot = NULL;

void usage(int ret) {
  FILE *stream = (ret ? stderr : stdout);
#define hputs(s) fputs(s"\n", stream);
  hputs("pacrepairdb - fix corrupted database entries");
  hputs("usage:  pacrepairdb [options] <package>...");
  hputs("        pacrepairdb (--help|--version)");
  hputs("");
  hputs("   --cachedir=<path>  set an alternate cache location");
  hputs("   --config=<path>    set an alternate configuration file");
  hputs("   --dbonly           update database without reinstalling packages");
  hputs("   --dbpath=<path>    set an alternate database location");
  hputs("   --debug            enable extra debugging messages");
  hputs("   --hookdir=<path>   add additional user hook directory");
  hputs("   --logfile=<path>   set an alternate log file");
  hputs("   --no-confirm       assume default responses to all prompts");
  hputs("   --no-hooks         do not run transaction hooks");
  hputs("   --no-scriptlet     do not run package install scripts");
  hputs("   --no-timeout       disable low speed timeouts for downloads");
  hputs("   --print-only       show steps without performing them");
  hputs("   --root=<path>      set an alternate installation root");
  hputs("   --sysroot=<path>   set an alternate system root");
  hputs("   --verbose          display additional progress information");
  hputs("");
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
    { "cache-dir", required_argument, NULL, FLAG_CACHEDIR     },
    { "cachedir", required_argument, NULL, FLAG_CACHEDIR     },
    { "config", required_argument, NULL, FLAG_CONFIG       },
    { "dbonly", no_argument, NULL, FLAG_DBONLY       },
    { "dbpath", required_argument, NULL, FLAG_DBPATH       },
    { "debug", optional_argument, NULL, FLAG_DEBUG        },
    { "hookdir", required_argument, NULL, FLAG_HOOKDIR      },
    { "logfile", required_argument, NULL, FLAG_LOGFILE      },
    { "no-confirm", no_argument, NULL, FLAG_NOCONFIRM    },
    { "noconfirm", no_argument, NULL, FLAG_NOCONFIRM    },
    { "no-scriptlet", no_argument, NULL, FLAG_NOSCRIPTLET  },
    { "no-hooks", no_argument, NULL, FLAG_NOHOOKS      },
    { "no-timeout", no_argument, NULL, FLAG_NOTIMEOUT    },
    { "print-only", no_argument, NULL, FLAG_PRINT        },
    { "root", required_argument, NULL, FLAG_ROOT         },
    { "sysroot", required_argument, NULL, FLAG_SYSROOT      },
    { "verbose", no_argument, NULL, FLAG_VERBOSE      },

    { "help", no_argument, NULL, FLAG_HELP         },
    { "version", no_argument, NULL, FLAG_VERSION      },

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

      /* general options */
      case FLAG_CACHEDIR:
        FREELIST(config->cachedirs);
        config->cachedirs = alpm_list_add(NULL, strdup(optarg));
        break;
      case FLAG_CONFIG:
        config_file = optarg;
        break;
      case FLAG_DBONLY:
        trans_flags |= ALPM_TRANS_FLAG_DBONLY;
        break;
      case FLAG_DBPATH:
        free(config->dbpath);
        config->dbpath = strdup(optarg);
        break;
      case FLAG_DEBUG:
        log_level |= ALPM_LOG_DEBUG;
        log_level |= ALPM_LOG_FUNCTION;
        break;
      case FLAG_HOOKDIR:
        alpm_list_append(&config->hookdirs, strdup(optarg));
        break;
      case FLAG_LOGFILE:
        free(config->logfile);
        config->logfile = strdup(optarg);
        break;
      case FLAG_NOCONFIRM:
        noconfirm = 1;
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
      case FLAG_PRINT:
        printonly = 1;
        break;
      case FLAG_ROOT:
        free(config->rootdir);
        config->rootdir = strdup(optarg);
        break;
      case FLAG_SYSROOT:
        sysroot = optarg;
        break;
      case FLAG_VERBOSE:
        verbose = 1;
        break;

      case FLAG_HELP:
        usage(0);
        break;
      case FLAG_VERSION:
        pu_print_version(myname, myver);
        exit(0);
        break;

      case '?':
      default:
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

/* gcc8's cast-function-type warning is a bit overzealous */
void free_pkg(alpm_pkg_t *p) {
  alpm_pkg_free(p);
}

alpm_list_t *load_cache_pkgs(alpm_handle_t *handle) {
  alpm_list_t *i, *cache_pkgs = NULL;
  puts("Loading cache packages...");
  for (i = alpm_option_get_cachedirs(handle); i; i = i->next) {
    const char *path = i->data;
    DIR *dir = opendir(path);
    struct dirent *entry;
    if (!dir) {
      pu_ui_warn("could not open cache dir '%s' (%s)", path, strerror(errno));
      continue;
    }
    errno = 0;
    while ((entry = readdir(dir))) {
      size_t path_len;
      char *filename, *name = entry->d_name;
      alpm_pkg_t *pkg = NULL;

      if (strcmp(".", name) == 0 || strcmp("..", name) == 0) {
        continue;
      }
      if (strlen(name) >= 4 && strcmp(name + strlen(name) - 4, ".sig") == 0) {
        continue;
      }

      path_len = strlen(path) + strlen(name);
      filename = malloc(path_len + 1);
      sprintf(filename, "%s%s", path, entry->d_name);
      if (alpm_pkg_load(handle, filename, 1, 0, &pkg) == 0) {
        alpm_list_append(&cache_pkgs, pkg);
      } else {
        pu_ui_warn("could not load package '%s' (%s)",
            filename, alpm_strerror(alpm_errno(handle)));
      }
      free(filename);
    }
    if (errno != 0) {
      pu_ui_warn("could not read cache dir '%s' (%s)", path, strerror(errno));
    }
    closedir(dir);
  }
  return cache_pkgs;
}

alpm_list_t *find_cached_pkg(alpm_pkg_t *pkg, alpm_list_t *cache_pkgs) {
  const char *name = alpm_pkg_get_name(pkg), *ver = alpm_pkg_get_version(pkg);
  alpm_list_t *i, *found = NULL;
  alpm_list_t *allowed_arch = alpm_option_get_architectures(handle);
  for (i = cache_pkgs; i; i = i->next) {
    if (strcmp(name, alpm_pkg_get_name(i->data)) != 0
        || strcmp(ver, alpm_pkg_get_version(i->data)) != 0) {
      /* name/version don't match */
      continue;
    } else if (alpm_pkg_get_arch(pkg)
        && strcmp(alpm_pkg_get_arch(pkg), alpm_pkg_get_arch(i->data)) != 0) {
      /* architecture doesn't match */
      continue;
    } else if (alpm_pkg_get_arch(pkg) == NULL
        && alpm_pkg_get_arch(i->data) != NULL
        && allowed_arch
        && !alpm_list_find_str(allowed_arch, alpm_pkg_get_arch(i->data))
        && strcmp(alpm_pkg_get_arch(i->data), "any") != 0) {
      /* needle has no architecture and package is not installable */
      continue;
    } else if (alpm_list_append(&found, i) == NULL) {
      /* package matched but we ran out of memory */
      pu_ui_error("%s", strerror(errno));
      alpm_list_free(found);
      return NULL;
    }
  }
  switch (alpm_list_count(found)) {
    case 0:
      pu_ui_warn("unable to locate cached package for '%s-%s'", name, ver);
      return NULL;
    case 1:
      i = found->data;
      alpm_list_free(found);
      return i;
    default:
      pu_ui_warn("multiple packages found for '%s-%s'", name, ver);
      for (i = found; i; i = i->next) {
        alpm_list_t *j = i->data;
        fprintf(stderr, "  %s\n", alpm_pkg_get_filename(j->data));
      }
      alpm_list_free(found);
      return NULL;
  }
}

void _pu_list_append_item(alpm_list_t **list, alpm_list_t *item) {
  item->next = NULL;

  /* Special case: the input list is empty */
  if (*list == NULL) {
    *list = item;
    item->prev = item;
  } else {
    alpm_list_t *lp = alpm_list_last(*list);
    lp->next = item;
    item->prev = lp;
    (*list)->prev = item;
  }
}

alpm_list_t *find_cached_pkgs(alpm_handle_t *handle, alpm_list_t *pkgnames) {
  alpm_list_t *i, *packages = NULL, *cache_files = load_cache_pkgs(handle);
  int error = 0;

  if (cache_files == NULL) {
    pu_ui_error("no cached packages found");
    return NULL;
  }

  for (i = pkgnames; i; i = i->next) {
    alpm_list_t *match = find_cached_pkg(i->data, cache_files);
    if (match == NULL) {
      error = 1;
      continue;
    } else {
      cache_files = alpm_list_remove_item(cache_files, match);
      _pu_list_append_item(&packages, match);
    }
  }

  alpm_list_free_inner(cache_files, (alpm_list_fn_free) free_pkg);
  alpm_list_free(cache_files);
  if (!error) {
    return packages;
  } else {
    alpm_list_free_inner(packages, (alpm_list_fn_free) free_pkg);
    alpm_list_free(packages);
    return NULL;
  }
}

int fix_db_file(int dirfd, const char *base, const char *file) {
  int fd;
  struct stat sbuf;
  if ((fd = openat(dirfd, file, O_RDONLY)) < 0 && errno == ENOENT) {
    if (verbose) { printf("creating empty db file '%s/%s'\n", base, file); }
    fd = openat(dirfd, file, O_RDONLY | O_CREAT, 0644);
  }
  if (fd < 0) {
    pu_ui_error("unable to open or create '%s/%s' (%s)",
        base, file, strerror(errno));
    return -1;
  }
  if (fstat(fd, &sbuf) != 0) {
    pu_ui_error("unable to stat '%s/%s' (%s)", base, file, strerror(errno));
    close(fd);
    return -1;
  }
  close(fd);
  if (!S_ISREG(sbuf.st_mode)) {
    pu_ui_error("database entry is not a file '%s/%s'", base, file);
    return -1;
  }
  return 0;
}

int fix_db_entry(alpm_handle_t *handle, alpm_pkg_t *pkg) {
  int dirfd;
  char dbdir[PATH_MAX];
  ssize_t len = snprintf(dbdir, PATH_MAX, "%slocal/%s-%s",
          alpm_option_get_dbpath(handle),
          alpm_pkg_get_name(pkg), alpm_pkg_get_version(pkg));

  if (len >= PATH_MAX) {
    /* this should not be possible */
    pu_ui_error("unable to open '%s/local/%s-%s' (%s)",
        alpm_option_get_dbpath(handle),
        alpm_pkg_get_name(pkg), alpm_pkg_get_version(pkg),
        strerror(ENAMETOOLONG));
    return -1;
  }

  if ((dirfd = open(dbdir, O_RDONLY | O_DIRECTORY)) < 0) {
    pu_ui_error("unable to open '%s' (%s)", dbdir, strerror(errno));
    return -1;
  }

  if (fix_db_file(dirfd, dbdir, "desc") != 0
      || fix_db_file(dirfd, dbdir, "files") != 0) {
    close(dirfd);
    return -1;
  }

  close(dirfd);
  return 0;
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

int install_packages(alpm_handle_t *handle, alpm_list_t *packages) {
  alpm_list_t *err_data, *i;
  int ret = 0;

  /* initialize the transaction first to lock the db */
  if (alpm_trans_init(handle, trans_flags) != 0) {
    fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
    return 1;
  }

  puts("Adding missing database files...");
  for (i = packages; i; i = i->next) {
    if (fix_db_entry(handle, i->data) != 0) {
      ret = 1;
      goto transcleanup;
    }
  }

  puts("Reinstalling packages...");
  for (i = packages; i; i = i->next) {
    if (alpm_add_pkg(handle, i->data) != 0) {
      fprintf(stderr, "%s\n", alpm_strerror(alpm_errno(handle)));
      ret = 1;
      goto transcleanup;
    }
  }

  if (alpm_trans_prepare(handle, &err_data) != 0) {
    alpm_errno_t err = alpm_errno(handle);
    switch (err) {
      case ALPM_ERR_PKG_INVALID_ARCH:
        for (i = err_data; i; i = alpm_list_next(i)) {
          char *pkgname = i->data;
          pu_ui_error("invalid architecture (%s)", pkgname);
          free(pkgname);
        }
        break;
      case ALPM_ERR_UNSATISFIED_DEPS:
        for (i = err_data; i; i = alpm_list_next(i)) {
          alpm_depmissing_t *dep = i->data;
          char *depstr = alpm_dep_compute_string(dep->depend);
          pu_ui_error("missing dependency '%s' (%s)", dep->target, depstr);
          free(depstr);
          alpm_depmissing_free(dep);
        }
        break;
      case ALPM_ERR_CONFLICTING_DEPS:
        for (i = err_data; i; i = alpm_list_next(i)) {
          alpm_conflict_t *conflict = i->data;
          pu_ui_error("package conflict (%s %s)",
              alpm_pkg_get_name(conflict->package1),
              alpm_pkg_get_name(conflict->package2));
          alpm_conflict_free(conflict);
        }
        break;
      default:
        pu_ui_error("%s", alpm_strerror(alpm_errno(handle)));
        break;
    }
    ret = 1;
    goto transcleanup;
  }

  if (alpm_trans_commit(handle, &err_data) != 0) {
    switch (alpm_errno(handle)) {
      case ALPM_ERR_PKG_INVALID:
      case ALPM_ERR_PKG_INVALID_CHECKSUM:
      case ALPM_ERR_PKG_INVALID_SIG:
        for (i = err_data; i; i = i->next) {
          char *path = i->data;
          pu_ui_error("%s is invalid or corrupted", path);
          free(path);
        }
        break;

      case ALPM_ERR_FILE_CONFLICTS:
        for (i = err_data; i; i = i->next) {
          print_fileconflict(i->data);
          alpm_fileconflict_free(i->data);
        }
        break;

      default:
        /* FIXME: possible memory leak */
        pu_ui_error("%s", alpm_strerror(alpm_errno(handle)));
        break;
    }

    alpm_list_free(err_data);
    err_data = NULL;
    ret = 1;
    goto transcleanup;
  }

transcleanup:
  if (alpm_trans_release(handle) != 0) {
    pu_ui_error("%s", alpm_strerror(alpm_errno(handle)));
    ret = 1;
  }

  return ret;
}

void cb_log(void *ctx, alpm_loglevel_t level, const char *fmt, va_list args) {
  (void)ctx;
  if (level & log_level) {
    vprintf(fmt, args);
  }
}

int main(int argc, char **argv) {
  int ret = 0;
  alpm_db_t *localdb;
  alpm_list_t *cache_pkgs = NULL, *packages = NULL, *i;
  int have_stdin = !isatty(fileno(stdin)) && errno != EBADF;

  if (!(config = parse_opts(argc, argv))) {
    ret = 1;
    goto cleanup;
  }

  if (!(handle = pu_initialize_handle_from_config(config))) {
    fprintf(stderr, "error: failed to initialize alpm.\n");
    ret = 1;
    goto cleanup;
  }

  if (nohooks) {
    alpm_option_set_hookdirs(handle, NULL);
  }
  alpm_option_set_questioncb(handle, pu_ui_cb_question, NULL);
  alpm_option_set_progresscb(handle, pu_ui_cb_progress, NULL);
  alpm_option_set_dlcb(handle, pu_ui_cb_download, NULL);
  alpm_option_set_logcb(handle, cb_log, NULL);
  alpm_option_add_overwrite_file(handle, "*");

  localdb = alpm_get_localdb(handle);
  while (optind < argc) {
    const char *pkgname = argv[optind];
    alpm_pkg_t *p;

    if (strncmp(pkgname, "local/", strlen("local/")) == 0) {
      pkgname += strlen("local/");
    }
    p = alpm_db_get_pkg(localdb, argv[optind]);
    if (p == NULL || alpm_list_append(&packages, p) == NULL) {
      pu_ui_error("unable to locate package '%s'", argv[optind]);
      ret = 1;
      goto cleanup;
    }
    optind++;
  }
  if (have_stdin) {
    char *buf = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&buf, &len, stdin)) != -1) {
      char *pkgname = buf;
      alpm_pkg_t *p;

      if (buf[read - 1] == '\n') { buf[read - 1] = '\0'; }
      if (strncmp(pkgname, "local/", strlen("local/")) == 0) {
        pkgname += strlen("local/");
      }
      p = alpm_db_get_pkg(localdb, pkgname);
      if (p == NULL || alpm_list_append(&packages, p) == NULL) {
        pu_ui_error("unable to locate package '%s'", argv[optind]);
        ret = 1;
        goto cleanup;
      }
    }
    free(buf);
  }

  if (packages == NULL) {
    pu_ui_error("no packages provided");
    ret = 1;
    goto cleanup;
  }

  if ((cache_pkgs = find_cached_pkgs(handle, packages)) == NULL) {
    ret = 1;
    goto cleanup;
  }

  puts("Attempting to repair the following packages:");
  for (i = cache_pkgs; i; i = i->next) {
    printf("  %s-%s (%s)\n",
        alpm_pkg_get_name(i->data), alpm_pkg_get_version(i->data),
        alpm_pkg_get_filename(i->data));
  }
  if (printonly || (!noconfirm && !pu_ui_confirm(1, "Proceed with repair?")) ) {
    goto cleanup;
  }

  pu_log_command(handle, log_prefix, argc, argv);
  ret = install_packages(handle, cache_pkgs);

cleanup:
  alpm_list_free(packages);
  alpm_list_free(cache_pkgs);
  alpm_release(handle);
  pu_config_free(config);

  return ret;
}
