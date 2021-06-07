/*
 * Copyright 2014-2016 Andrew Gregory <andrew.gregory.8@gmail.com>
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
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#include <pacutils.h>

#include "config-defaults.h"

const char *myname = "paccheck", *myver = BUILDVER;

enum longopt_flags {
  FLAG_ADD = 1000,
  FLAG_BACKUP,
  FLAG_CONFIG,
  FLAG_DB_FILES,
  FLAG_DBPATH,
  FLAG_DEPENDS,
  FLAG_FILES,
  FLAG_FILE_PROPERTIES,
  FLAG_HELP,
  FLAG_LIST_BROKEN,
  FLAG_MD5SUM,
  FLAG_SHA256SUM,
  FLAG_NOEXTRACT,
  FLAG_NOUPGRADE,
  FLAG_NULL,
  FLAG_OPT_DEPENDS,
  FLAG_QUIET,
  FLAG_RECURSIVE,
  FLAG_REQUIRE_MTREE,
  FLAG_ROOT,
  FLAG_SYSROOT,
  FLAG_VERSION,
};

enum check_types {
  CHECK_DEPENDS = 1 << 0,
  CHECK_OPT_DEPENDS = 1 << 1,
  CHECK_FILES = 1 << 2,
  CHECK_FILE_PROPERTIES = 1 << 3,
  CHECK_MD5SUM = 1 << 4,
  CHECK_SHA256SUM = 1 << 5,
};

pu_config_t *config = NULL;
alpm_handle_t *handle = NULL;
alpm_db_t *localdb = NULL;
alpm_list_t *pkgcache = NULL, *packages = NULL;
const char *sysroot = NULL;
int checks = 0, recursive = 0, list_broken = 0, quiet = 0;
int include_db_files = 0, require_mtree = 0;
int skip_backups = 1, skip_noextract = 1, skip_noupgrade = 1;
int isep = '\n';

void usage(int ret) {
  FILE *stream = (ret ? stderr : stdout);
#define hputs(s) fputs(s"\n", stream);
  hputs("paccheck - check installed packages");
  hputs("usage:  paccheck [options] [<package>]...");
  hputs("        paccheck (--help|--version)");
  hputs("");
  hputs("   --config=<path>    set an alternate configuration file");
  hputs("   --dbpath=<path>    set an alternate database location");
  hputs("   --root=<path>      set an alternate installation root");
  hputs("   --sysroot=<path>   set an alternate system root");
  hputs("   --null[=<sep>]     parse stdin as <sep> separated values (default NUL)");
  hputs("   --list-broken      only print packages that fail checks");
  hputs("   --quiet            only display error messages");
  hputs("   --help             display this help information");
  hputs("   --version          display version information");
  hputs("");
  hputs("   --recursive        perform checks on package [opt-]depends");
  hputs("   --require-mtree    treat packages missing MTREE data as an error");
  hputs("   --depends          check for missing dependencies");
  hputs("   --opt-depends      check for missing optional dependencies");
  hputs("   --files            check installed files against package database");
  hputs("   --file-properties  check installed files against MTREE data");
  hputs("   --md5sum           check file md5sums against MTREE data");
  hputs("   --sha256sum        check file sha256sums against MTREE data");
  hputs("   --backup           include backup files in modification checks");
  hputs("   --noextract        include NoExtract files in modification checks");
  hputs("   --noupgrade        include NoUpgrade files in modification checks");
  hputs("   --db-files         include database files in checks");
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
    { "dbpath", required_argument, NULL, FLAG_DBPATH       },
    { "root", required_argument, NULL, FLAG_ROOT         },
    { "sysroot", required_argument, NULL, FLAG_SYSROOT      },
    { "quiet", no_argument, NULL, FLAG_QUIET        },
    { "null", optional_argument, NULL, FLAG_NULL         },

    { "help", no_argument, NULL, FLAG_HELP         },
    { "version", no_argument, NULL, FLAG_VERSION      },

    { "recursive", no_argument, NULL, FLAG_RECURSIVE    },

    { "backup", no_argument, NULL, FLAG_BACKUP       },
    { "noextract", no_argument, NULL, FLAG_NOEXTRACT    },
    { "noupgrade", no_argument, NULL, FLAG_NOUPGRADE    },
    { "db-files", no_argument, NULL, FLAG_DB_FILES     },

    { "depends", no_argument, NULL, FLAG_DEPENDS      },
    { "opt-depends", no_argument, NULL, FLAG_OPT_DEPENDS  },
    { "files", no_argument, NULL, FLAG_FILES        },
    { "file-properties", no_argument, NULL, FLAG_FILE_PROPERTIES },
    { "mtree", no_argument, NULL, FLAG_FILE_PROPERTIES },
    { "require-mtree", no_argument, NULL, FLAG_REQUIRE_MTREE },
    { "list-broken", no_argument, NULL, FLAG_LIST_BROKEN  },
    { "md5sum", no_argument, NULL, FLAG_MD5SUM       },
    { "sha256sum", no_argument, NULL, FLAG_SHA256SUM    },

    { 0, 0, 0, 0 },
  };

  if ((config = pu_config_new()) == NULL) {
    perror("malloc");
    return NULL;
  }

  /* process remaining command-line options */
  optind = 1;
  while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
    switch (c) {

      case 0:
        /* already handled */
        break;

      /* general options */
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
      case FLAG_DBPATH:
        free(config->dbpath);
        config->dbpath = strdup(optarg);
        break;
      case FLAG_QUIET:
        quiet = 1;
        break;
      case FLAG_NULL:
        isep = optarg ? optarg[0] : '\0';
        break;
      case FLAG_ROOT:
        free(config->rootdir);
        config->rootdir = strdup(optarg);
        break;
      case FLAG_SYSROOT:
        sysroot = optarg;
        break;

      /* checks */
      case FLAG_DEPENDS:
        checks |= CHECK_DEPENDS;
        break;
      case FLAG_OPT_DEPENDS:
        checks |= CHECK_OPT_DEPENDS;
        break;
      case FLAG_FILES:
        checks |= CHECK_FILES;
        break;
      case FLAG_FILE_PROPERTIES:
        checks |= CHECK_FILE_PROPERTIES;
        break;
      case FLAG_MD5SUM:
        checks |= CHECK_MD5SUM;
        break;
      case FLAG_SHA256SUM:
        checks |= CHECK_SHA256SUM;
        break;

      /* misc */
      case FLAG_RECURSIVE:
        recursive = 1;
        break;
      case FLAG_REQUIRE_MTREE:
        require_mtree = 1;
        break;
      case FLAG_DB_FILES:
        include_db_files = 1;
        break;
      case FLAG_BACKUP:
        skip_backups = 0;
        break;
      case FLAG_LIST_BROKEN:
        list_broken = 1;
        break;
      case FLAG_NOEXTRACT:
        skip_noextract = 0;
        break;
      case FLAG_NOUPGRADE:
        skip_noupgrade = 0;
        break;

      case '?':
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

#define match_noupgrade(h, p) (alpm_option_match_noupgrade(h, p) == 0)
#define match_noextract(h, p) (alpm_option_match_noextract(h, p) == 0)
static int match_backup(alpm_pkg_t *pkg, const char *path) {
  alpm_list_t *i;
  for (i = alpm_pkg_get_backup(pkg); i; i = alpm_list_next(i)) {
    alpm_backup_t *b = i->data;
    if (strcmp(path, b->name) == 0) {
      return 1;
    }
  }
  return 0;
}

static void eprintf(const char *fmt, ...) {
  if (!list_broken) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
  }
}

static int check_depends(alpm_pkg_t *p) {
  int ret = 0;
  alpm_list_t *i;
  for (i = alpm_pkg_get_depends(p); i; i = alpm_list_next(i)) {
    char *depstring = alpm_dep_compute_string(i->data);
    if (!alpm_find_satisfier(pkgcache, depstring)) {
      eprintf("%s: unsatisfied dependency '%s'\n",
          alpm_pkg_get_name(p), depstring);
      ret = 1;
    }
    free(depstring);
  }
  if (!quiet && !ret) {
    eprintf("%s: all dependencies satisfied\n", alpm_pkg_get_name(p));
  }
  return ret;
}

static int check_opt_depends(alpm_pkg_t *p) {
  int ret = 0;
  alpm_list_t *i;
  for (i = alpm_pkg_get_optdepends(p); i; i = alpm_list_next(i)) {
    char *depstring = alpm_dep_compute_string(i->data);
    if (!alpm_find_satisfier(pkgcache, depstring)) {
      eprintf("%s: unsatisfied optional dependency '%s'\n",
          alpm_pkg_get_name(p), depstring);
      ret = 1;
    }
    free(depstring);
  }
  if (!quiet && !ret) {
    eprintf("%s: all optional dependencies satisfied\n",
        alpm_pkg_get_name(p));
  }
  return ret;
}

static int check_file(const char *pkgname, const char *path, int isdir) {
  struct stat buf;
  if (lstat(path, &buf) != 0) {
    if (errno == ENOENT) {
      eprintf("%s: '%s' missing file\n", pkgname, path);
    } else {
      pu_ui_warn("%s: '%s' read error (%s)", pkgname, path, strerror(errno));
    }
    return 1;
  } else if (isdir && !S_ISDIR(buf.st_mode)) {
    eprintf("%s: '%s' type mismatch (expected directory)\n", pkgname, path);
    return 1;
  } else if (!isdir && S_ISDIR(buf.st_mode)) {
    eprintf("%s: '%s' type mismatch (expected file)\n", pkgname, path);
    return 1;
  }
  return 0;
}

static char *get_db_path(alpm_pkg_t *pkg, const char *path) {
  static char dbpath[PATH_MAX];
  ssize_t len = snprintf(dbpath, PATH_MAX, "%slocal/%s-%s/%s",
          alpm_option_get_dbpath(handle),
          alpm_pkg_get_name(pkg), alpm_pkg_get_version(pkg), path);
  if (len < 0 || len >= PATH_MAX) { errno = ERANGE; return NULL; } /* TODO */
  return dbpath;
}

/* verify that required db files exist */
static int check_db_files(alpm_pkg_t *pkg) {
  const char *pkgname = alpm_pkg_get_name(pkg);
  char *dbpath;
  int ret = 0;

  if ((dbpath = get_db_path(pkg, "desc")) == NULL) {
    pu_ui_warn("%s: '%s' read error (%s)", pkgname, dbpath, strerror(errno));
  } else if (check_file(pkgname, dbpath, 0) != 0) {
    ret = 1;
  }

  if ((dbpath = get_db_path(pkg, "files")) == NULL) {
    pu_ui_warn("%s: '%s' read error (%s)", pkgname, dbpath, strerror(errno));
  } else if (check_file(pkgname, dbpath, 0) != 0) {
    ret = 1;
  }

  if (!require_mtree) { return ret; }

  if ((dbpath = get_db_path(pkg, "mtree")) == NULL) {
    pu_ui_warn("%s: '%s' read error (%s)", pkgname, dbpath, strerror(errno));
  } else if (check_file(pkgname, dbpath, 0) != 0) {
    ret = 1;
  }

  return ret;
}

/* verify that the filesystem matches the package database */
static int check_files(alpm_pkg_t *pkg) {
  alpm_filelist_t *filelist = alpm_pkg_get_files(pkg);
  char path[PATH_MAX], *rel;
  const char *pkgname = alpm_pkg_get_name(pkg);
  unsigned int i;
  int ret = 0;
  size_t space;

  strncpy(path, alpm_option_get_root(handle), PATH_MAX);
  rel = path + strlen(path);
  space = PATH_MAX - (rel - path);

  for (i = 0; i < filelist->count; ++i) {
    alpm_file_t file = filelist->files[i];
    int isdir = 0;
    size_t len;

    if (skip_noextract && match_noextract(handle, file.name)) { continue; }

    strncpy(rel, file.name, space);
    len = strlen(file.name);
    if (rel[len - 1] == '/') {
      isdir = 1;
      rel[len - 1] = '\0';
    }

    if (check_file(pkgname, path, isdir) != 0) { ret = 1; }
  }

  if (include_db_files && check_db_files(pkg) != 0) {
    ret = 1;
  }

  if (!quiet && !ret) {
    eprintf("%s: all files match database\n", alpm_pkg_get_name(pkg));
  }

  return ret;
}

const char *mode_str(mode_t mode) {
  if (S_ISREG(mode)) {
    return "file";
  } else if (S_ISDIR(mode)) {
    return "directory";
  } else if (S_ISBLK(mode)) {
    return "block";
  } else if (S_ISCHR(mode)) {
    return "character special file";
  } else if (S_ISFIFO(mode)) {
    return "fifo";
  } else if (S_ISLNK(mode)) {
    return "link";
  } else if (S_ISSOCK(mode)) {
    return "socket";
  } else {
    return "unknown type";
  }
}

int cmp_type(alpm_pkg_t *pkg, const char *path,
    struct archive_entry *entry, struct stat *st) {
  const char *type = mode_str(archive_entry_filetype(entry));
  const char *ftype = mode_str(st->st_mode);

  if (type != ftype) {
    eprintf("%s: '%s' type mismatch (expected %s)\n",
        alpm_pkg_get_name(pkg), path, type);
    return 1;
  }

  return 0;
}

int cmp_mode(alpm_pkg_t *pkg, const char *path,
    struct archive_entry *entry, struct stat *st) {
  mode_t mask = 07777;
  mode_t perm = archive_entry_perm(entry);

  if (perm != (st->st_mode & mask)) {
    eprintf("%s: '%s' permission mismatch (expected %o)\n",
        alpm_pkg_get_name(pkg), path, perm);
    return 1;
  }

  return 0;
}

int cmp_mtime(alpm_pkg_t *pkg, const char *path,
    struct archive_entry *entry, struct stat *st) {
  time_t t = archive_entry_mtime(entry);

  if (t != st->st_mtime) {
    struct tm ltime;
    char time_buf[26];
    strftime(time_buf, 26, "%F %T", localtime_r(&t, &ltime));
    eprintf("%s: '%s' modification time mismatch (expected %s)\n",
        alpm_pkg_get_name(pkg), path, time_buf);
    return 1;
  }

  return 0;
}

int cmp_target(alpm_pkg_t *pkg, const char *path,
    struct archive_entry *entry) {
  const char *ptarget = archive_entry_symlink(entry);
  char ftarget[PATH_MAX];
  ssize_t len = readlink(path, ftarget, PATH_MAX);
  ftarget[len] = '\0';

  if (strcmp(ptarget, ftarget) != 0) {
    eprintf("%s: '%s' symlink target mismatch (expected %s)\n",
        alpm_pkg_get_name(pkg), path, ptarget);
    return 1;
  }

  return 0;
}

int cmp_uid(alpm_pkg_t *pkg, const char *path,
    struct archive_entry *entry, struct stat *st) {
  uid_t puid = archive_entry_uid(entry);

  if (puid != st->st_uid) {
    struct passwd *pw = getpwuid(puid);
    eprintf("%s: '%s' UID mismatch (expected %d/%s)\n",
        alpm_pkg_get_name(pkg), path, puid, pw ? pw->pw_name : "unknown user");
    return 1;
  }

  return 0;
}

int cmp_gid(alpm_pkg_t *pkg, const char *path,
    struct archive_entry *entry, struct stat *st) {
  gid_t pgid = archive_entry_gid(entry);

  if (pgid != st->st_gid) {
    struct group *gr = getgrgid(pgid);
    eprintf("%s: '%s' GID mismatch (expected %d/%s)\n",
        alpm_pkg_get_name(pkg), path, pgid, gr ? gr->gr_name : "unknown group");
    return 1;
  }

  return 0;
}

int cmp_size(alpm_pkg_t *pkg, const char *path,
    struct archive_entry *entry, struct stat *st) {
  int64_t psize = archive_entry_size(entry);

  if (psize != st->st_size) {
    char hr_size[20];
    eprintf("%s: '%s' size mismatch (expected %s)\n",
        alpm_pkg_get_name(pkg), path, pu_hr_size(psize, hr_size));
    return 1;
  }

  return 0;
}

/* check filesystem against extra mtree data if available,
 * NOT guaranteed to catch db/filesystem discrepencies */
static int check_file_properties(alpm_pkg_t *pkg) {
  char path[PATH_MAX], *rel;
  int ret = 0;
  size_t space;
  struct archive *mtree = alpm_pkg_mtree_open(pkg);
  struct archive_entry *entry;

  if (!mtree) {
    pu_ui_warn("%s: mtree data not available (%s)",
        alpm_pkg_get_name(pkg), strerror(errno));
    return require_mtree;
  }

  strncpy(path, alpm_option_get_root(handle), PATH_MAX);
  rel = path + strlen(path);
  space = PATH_MAX - (rel - path);

  while (alpm_pkg_mtree_next(pkg, mtree, &entry) == ARCHIVE_OK) {
    const char *ppath = archive_entry_pathname(entry);
    const char *fpath;
    struct stat buf;

    if (strncmp("./", ppath, 2) == 0) { ppath += 2; }

    if (strcmp(ppath, ".INSTALL") == 0) {
      if ((fpath = get_db_path(pkg, "install")) == NULL) {
        continue;
      }
    } else if (strcmp(ppath, ".CHANGELOG") == 0) {
      if ((fpath = get_db_path(pkg, "changelog")) == NULL) {
        continue;
      }
    } else if (ppath[0] == '.') {
      continue;
    } else if (skip_noextract && match_noextract(handle, ppath)) {
      continue;
    } else {
      strncpy(rel, ppath, space);
      fpath = path;
    }

    if (lstat(fpath, &buf) != 0) {
      if (errno == ENOENT) {
        eprintf("%s: '%s' missing file\n", alpm_pkg_get_name(pkg), fpath);
      } else {
        pu_ui_warn("%s: '%s' read error (%s)",
            alpm_pkg_get_name(pkg), fpath, strerror(errno));
      }
      ret = 1;
      continue;
    }

    if (cmp_type(pkg, fpath, entry, &buf) != 0) { ret = 1; }

    if (skip_noupgrade && match_noupgrade(handle, ppath)) { continue; }

    if (cmp_mode(pkg, fpath, entry, &buf) != 0) { ret = 1; }
    if (cmp_uid(pkg, fpath, entry, &buf) != 0) { ret = 1; }
    if (cmp_gid(pkg, fpath, entry, &buf) != 0) { ret = 1; }

    if (skip_backups && match_backup(pkg, ppath)) {
      continue;
    }

    if (S_ISLNK(buf.st_mode) && S_ISLNK(archive_entry_mode(entry))) {
      if (cmp_target(pkg, fpath, entry) != 0) { ret = 1; }
    }
    if (!S_ISDIR(buf.st_mode)) {
      if (cmp_mtime(pkg, fpath, entry, &buf) != 0) { ret = 1; }
      if (!S_ISLNK(buf.st_mode)) {
        /* always fails for directories and symlinks */
        if (cmp_size(pkg, fpath, entry, &buf) != 0) { ret = 1; }
      }
    }
  }
  alpm_pkg_mtree_close(pkg, mtree);

  if (!quiet && !ret) {
    eprintf("%s: all files match mtree\n", alpm_pkg_get_name(pkg));
  }

  return ret;
}

static int check_md5sum(alpm_pkg_t *pkg) {
  int ret = 0;
  char path[PATH_MAX], *rel;
  pu_mtree_reader_t *reader;
  pu_mtree_t *m;

  strcpy(path, alpm_option_get_root(handle));
  rel = path + strlen(alpm_option_get_root(handle));

  if ((reader = pu_mtree_reader_open_package(handle, pkg)) == NULL) {
    pu_ui_warn("%s: mtree data not available (%s)",
        alpm_pkg_get_name(pkg), strerror(errno));
    return require_mtree;
  }

  if ((m = pu_mtree_new()) == NULL) {
    pu_ui_warn("%s: error reading mtree data (%s)",
        alpm_pkg_get_name(pkg), strerror(errno));
    pu_mtree_reader_free(reader);
    return require_mtree;
  }

  while (pu_mtree_reader_next(reader, m)) {
    char *md5;
    if (m->md5digest[0] == '\0') { continue; }
    if (m->path[0] == '.') { continue; }
    if (skip_backups && match_backup(pkg, m->path)) { continue; }
    if (skip_noextract && match_noextract(handle, m->path)) { continue; }
    if (skip_noupgrade && match_noupgrade(handle, m->path)) { continue; }

    strcpy(rel, m->path);
    if ((md5 = alpm_compute_md5sum(path)) == NULL) {
      pu_ui_warn("%s: '%s' read error (%s)",
          alpm_pkg_get_name(pkg), path, strerror(errno));
    } else if (memcmp(m->md5digest, md5, 32) != 0) {
      eprintf("%s: '%s' md5sum mismatch (expected %s)\n",
          alpm_pkg_get_name(pkg), path, m->md5digest);
      ret = 1;
    }
    free(md5);
  }
  pu_mtree_free(m);

  if (!reader->eof) {
    pu_ui_warn("%s: error reading mtree data (%s)",
        alpm_pkg_get_name(pkg), strerror(errno));
    pu_mtree_reader_free(reader);
    return ret || require_mtree;
  }
  pu_mtree_reader_free(reader);

  if (!quiet && !ret) {
    eprintf("%s: all files match mtree md5sums\n", alpm_pkg_get_name(pkg));
  }

  return ret;
}

static int check_sha256sum(alpm_pkg_t *pkg) {
  int ret = 0;
  char path[PATH_MAX], *rel;
  pu_mtree_reader_t *reader;
  pu_mtree_t *m;

  if ((reader = pu_mtree_reader_open_package(handle, pkg)) == NULL) {
    pu_ui_warn("%s: mtree data not available (%s)",
        alpm_pkg_get_name(pkg), strerror(errno));
    return require_mtree;
  }

  strcpy(path, alpm_option_get_root(handle));
  rel = path + strlen(alpm_option_get_root(handle));

  if ((m = pu_mtree_new()) == NULL) {
    pu_ui_warn("%s: error reading mtree data (%s)",
        alpm_pkg_get_name(pkg), strerror(errno));
    pu_mtree_reader_free(reader);
    return require_mtree;
  }

  while (pu_mtree_reader_next(reader, m)) {
    char *sha;
    if (m->sha256digest[0] == '\0') { continue; }
    if (m->path[0] == '.') { continue; }
    if (skip_backups && match_backup(pkg, m->path)) { continue; }
    if (skip_noextract && match_noextract(handle, m->path)) { continue; }
    if (skip_noupgrade && match_noupgrade(handle, m->path)) { continue; }

    strcpy(rel, m->path);
    if ((sha = alpm_compute_sha256sum(path)) == NULL) {
      pu_ui_warn("%s: '%s' read error (%s)",
          alpm_pkg_get_name(pkg), path, strerror(errno));
    } else if (memcmp(m->sha256digest, sha, 32) != 0) {
      eprintf("%s: '%s' sha256sum mismatch (expected %s)\n",
          alpm_pkg_get_name(pkg), path, m->sha256digest);
      ret = 1;
    }
    free(sha);
  }
  pu_mtree_free(m);

  if (!reader->eof) {
    pu_ui_warn("%s: error reading mtree data (%s)",
        alpm_pkg_get_name(pkg), strerror(errno));
    pu_mtree_reader_free(reader);
    return ret || require_mtree;
  }
  pu_mtree_reader_free(reader);

  if (!quiet && !ret) {
    eprintf("%s: all files match mtree sha256sums\n", alpm_pkg_get_name(pkg));
  }

  return ret;
}

alpm_list_t *list_add_unique(alpm_list_t *list, void *data) {
  if (alpm_list_find_ptr(list, data)) {
    return list;
  } else {
    return alpm_list_add(list, data);
  }
}

alpm_pkg_t *load_pkg(const char *pkgname) {
  /* TODO pkgspec handling */
  if (strncmp(pkgname, "local/", 6) == 0) {
    pkgname += 6;
  }
  alpm_pkg_t *p = alpm_db_get_pkg(localdb, pkgname);
  if (p == NULL) {
    fprintf(stderr, "error: could not find package '%s'\n", pkgname);
  } else {
    packages = list_add_unique(packages, p);
  }
  return p;
}

void add_deps(alpm_pkg_t *pkg) {
  alpm_list_t *i;
  for (i = alpm_pkg_get_depends(pkg); i; i = alpm_list_next(i)) {
    char *depstring = alpm_dep_compute_string(i->data);
    alpm_pkg_t *p = alpm_find_satisfier(pkgcache, depstring);
    if (p && !alpm_list_find_ptr(packages, p)) {
      packages = alpm_list_add(packages, p);
      add_deps(p);
    }
    free(depstring);
  }
  if (checks & CHECK_OPT_DEPENDS) {
    for (i = alpm_pkg_get_optdepends(pkg); i; i = alpm_list_next(i)) {
      char *depstring = alpm_dep_compute_string(i->data);
      alpm_pkg_t *p = alpm_find_satisfier(pkgcache, depstring);
      if (p && !alpm_list_find_ptr(packages, p)) {
        packages = alpm_list_add(packages, p);
        add_deps(p);
      }
      free(depstring);
    }
  }
}

int main(int argc, char **argv) {
  alpm_list_t *i;
  int ret = 0;
  int have_stdin = !isatty(fileno(stdin)) && errno != EBADF;

  if (!(config = parse_opts(argc, argv))) {
    ret = 1;
    goto cleanup;
  }

  if (checks == 0) {
    checks = CHECK_DEPENDS | CHECK_FILES;
  }

  if (!(handle = pu_initialize_handle_from_config(config))) {
    fprintf(stderr, "error: failed to initialize alpm.\n");
    ret = 1;
    goto cleanup;
  }

  localdb = alpm_get_localdb(handle);
  pkgcache = alpm_db_get_pkgcache(localdb);

  for (; optind < argc; ++optind) {
    if (load_pkg(argv[optind]) == NULL) { ret = 1; }
  }
  if (have_stdin) {
    char *buf = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getdelim(&buf, &len, isep, stdin)) != -1) {
      if (buf[read - 1] == isep) { buf[read - 1] = '\0'; }
      if (load_pkg(buf) == NULL) { ret = 1; }
    }
    free(buf);
  }

  if (ret) { goto cleanup; }

  if (packages == NULL) {
    packages = alpm_list_copy(pkgcache);
    recursive = 0;
  } else if (recursive) {
    /* load [opt-]depends */
    alpm_list_t *i, *originals = alpm_list_copy(packages);
    for (i = originals; i; i = alpm_list_next(i)) {
      add_deps(i->data);
    }
    alpm_list_free(originals);
  }

  for (i = packages; i; i = alpm_list_next(i)) {
    int pkgerr = 0;
#define RUNCHECK(t, b) if((checks & t) && b != 0) { pkgerr = ret = 1; }
    RUNCHECK(CHECK_DEPENDS, check_depends(i->data));
    RUNCHECK(CHECK_OPT_DEPENDS, check_opt_depends(i->data));
    RUNCHECK(CHECK_FILES, check_files(i->data));
    RUNCHECK(CHECK_FILE_PROPERTIES, check_file_properties(i->data));
    RUNCHECK(CHECK_MD5SUM, check_md5sum(i->data));
    RUNCHECK(CHECK_SHA256SUM, check_sha256sum(i->data));
#undef RUNCHECK
    if (pkgerr && list_broken) { printf("%s\n", alpm_pkg_get_name(i->data)); }
  }

cleanup:
  alpm_list_free(packages);
  alpm_release(handle);
  pu_config_free(config);

  return ret;
}

/* vim: set ts=2 sw=2 noet: */
