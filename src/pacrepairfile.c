/*
 * Copyright 2015-2016 Andrew Gregory <andrew.gregory.8@gmail.com>
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

#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <grp.h>
#include <libgen.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <pacutils.h>

#include "config-defaults.h"

const char *myname = "pacrepairfile", *myver = BUILDVER;

enum longopt_flags {
  FLAG_CONFIG = 1000,
  FLAG_DBPATH,
  FLAG_HELP,
  FLAG_PACKAGE,
  FLAG_ROOT,
  FLAG_SYSROOT,
  FLAG_VERSION,
};

pu_config_t *config = NULL;
alpm_handle_t *handle = NULL;
alpm_list_t *packages = NULL;
int _fix_gid = 0, _fix_mode = 0, _fix_mtime = 0, _fix_uid = 0;
int verbose = 1;
const char *sysroot = NULL;

#define ASSERT(x) \
  if(!(x)) { pu_ui_error("unexpected error (%s)", strerror(errno)); exit(1); }

void usage(int ret) {
  FILE *stream = (ret ? stderr : stdout);
#define hputs(s) fputs(s"\n", stream);
  hputs("pacrepairfile - reset properties on alpm-managed files");
  hputs("usage:  pacrepairfile [options] <file>...");
  hputs("        pacrepairfile (--help|--version)");
  hputs("");
  hputs("   --config=<path>    set an alternate configuration file");
  hputs("   --dbpath=<path>    set an alternate database location");
  hputs("   --root=<path>      set an alternate installation root");
  hputs("   --sysroot=<path>   set an alternate system root");
  hputs("   --quiet            do not display progress information");
  hputs("   --help             display this help information");
  hputs("   --version          display version information");
  hputs("");
  hputs("   --package=<name>   search package <name> for file properties");
  hputs("   --uid              set file owner id");
  hputs("   --gid              set file group id");
  hputs("   --mode             set file permissions");
  hputs("   --mtime            set file modification time");
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

    { "quiet", no_argument, &verbose, 0                 },

    { "help", no_argument, NULL, FLAG_HELP         },
    { "version", no_argument, NULL, FLAG_VERSION      },

    { "package", required_argument, NULL, FLAG_PACKAGE      },

    { "uid", no_argument, &_fix_uid, 1                 },
    { "gid", no_argument, &_fix_gid, 1                 },
    { "mode", no_argument, &_fix_mode, 1                 },
    { "mtime", no_argument, &_fix_mtime, 1                 },

    { 0, 0, 0, 0 },
  };

  ASSERT(config = pu_config_new());

  /* process remaining command-line options */
  while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
    switch (c) {

      /* already handled */
      case 0:
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
        ASSERT(config->dbpath = strdup(optarg));
        break;
      case FLAG_ROOT:
        free(config->rootdir);
        ASSERT(config->rootdir = strdup(optarg));
        break;
      case FLAG_SYSROOT:
        sysroot = optarg;
        break;

      /* misc */
      case FLAG_PACKAGE:
        ASSERT(alpm_list_append(&packages, optarg));
        break;

      case '?':
      default:
        usage(1);
        break;
    }
  }

  if (!pu_ui_config_load_sysroot(config, config_file, sysroot)) {
    pu_ui_error("could not parse '%s'", config_file);
    return NULL;
  }

  return config;
}

int _fchmodat(int fd, const char *path, mode_t mode, int flag) {
  int ret = fchmodat(fd, path, mode, flag);
  if (ret != 0 && flag & AT_SYMLINK_NOFOLLOW && errno == ENOTSUP) {
    /* Linux does not provide a proper fchmodat with AT_SYMLINK_NOFOLLOW
     * support and pre-emptively fails if it's used; fall back to a more
     * portable method */
    int ofd = openat(fd, path, O_RDONLY | O_NOFOLLOW);
    if (ofd == -1) { errno = EOPNOTSUPP; return -1; }
    ret = fchmod(ofd, mode);
    close(ofd);
  }
  return ret;
}

int fix_mode(const char *path, struct archive_entry *entry) {
  mode_t m = archive_entry_perm(entry);
  if (_fchmodat(AT_FDCWD, path, m, AT_SYMLINK_NOFOLLOW) != 0) {
    pu_ui_warn("%s: unable to set permissions (%s)", path, strerror(errno));
    return 1;
  } else if (verbose) {
    printf("%s: set permissions to %o\n", path, m);
  }
  return 0;
}

int fix_mtime(const char *path, struct archive_entry *entry) {
  time_t t = archive_entry_mtime(entry);
  struct timespec times[2] = { { 0, UTIME_OMIT }, { t, 0 } };

  if (utimensat(AT_FDCWD, path, times, AT_SYMLINK_NOFOLLOW) != 0) {
    pu_ui_warn("%s: unable to set modification time (%s)",
        path, strerror(errno));
    return 1;
  } else if (verbose) {
    printf("%s: set modification time to %zu\n", path, t);
  }

  return 0;
}

int fix_uid(const char *path, struct archive_entry *entry) {
  uid_t u = archive_entry_uid(entry);
  if (fchownat(AT_FDCWD, path, u, -1, AT_SYMLINK_NOFOLLOW) != 0) {
    pu_ui_warn("%s: unable to set uid (%s)", path, strerror(errno));
    return 1;
  } else if (verbose) {
    printf("%s: set uid to %u\n", path, u);
  }

  return 0;
}

int fix_gid(const char *path, struct archive_entry *entry) {
  gid_t g = archive_entry_gid(entry);
  if (fchownat(AT_FDCWD, path, -1, g, AT_SYMLINK_NOFOLLOW) != 0) {
    pu_ui_warn("%s: unable to set gid (%s)", path, strerror(errno));
    return 1;
  } else if (verbose) {
    printf("%s: set gid to %u\n", path, g);
  }

  return 0;
}

char *lrealpath(const char *path, char *resolved_path) {
  /* copy path because dirname and basename do funky things */
  char *p = strdup(path);
  char *p2 = strdup(path);
  const char *bname = basename(p2), *dname = dirname(p);
  int alloc_path = (resolved_path == NULL);

  /* handle a few special cases of bname
   * that require resolving the entire path */
  if (strcmp(bname, "..") == 0
      || strcmp(bname, ".") == 0
      || strcmp(bname, "/") == 0) {
    dname = path;
    bname = NULL;
  }

  if ((resolved_path = realpath(dname, resolved_path)) == NULL) {
    free(p);
    free(p2);
    return NULL;
  }

  /* add bname back if we have a valid one */
  if (bname) {
    size_t rlen = strlen(resolved_path), blen = strlen(bname);
    if (alloc_path) {
      if (rlen + blen + 2 <= rlen) {
        errno = ENAMETOOLONG;
        free(p);
        free(p2);
        return NULL;
      } else {
        char *newp = realloc(resolved_path, rlen + blen + 2);
        if (newp == NULL) {
          errno = ENOMEM;
          free(p);
          free(p2);
          return NULL;
        }
        resolved_path = newp;
      }
    } else {
      /* error out if resolved_path is too long to append bname */
      if ((rlen + blen + 2) >= PATH_MAX) {
        errno = ENAMETOOLONG;
        free(p);
        free(p2);
        return NULL;
      }
    }
    if (resolved_path[rlen - 1] != '/') { strcpy(resolved_path + rlen++, "/"); }
    strcpy(resolved_path + rlen, bname);
  }

  free(p);
  free(p2);
  return resolved_path;
}

int fix_file(const char *file) {
  alpm_list_t *i;
  char *rpath = lrealpath(file, NULL);
  const char *root = alpm_option_get_root(handle);
  size_t rootlen = strlen(root);

  if (rpath == NULL) {
    pu_ui_error("%s: unable to resolve path (%s)", file, strerror(errno));
    return 1;
  }

  if (strncmp(rpath, root, rootlen) != 0) {
    pu_ui_error("%s: file not owned");
    free(rpath);
    return 1;
  }

  if (access(rpath, W_OK)) {
    pu_ui_error("%s: %s", rpath, strerror(errno));
    free(rpath);
    return 1;
  }

  for (i = packages; i; i = alpm_list_next(i)) {
    alpm_filelist_t *filelist = alpm_pkg_get_files(i->data);
    if (pu_filelist_contains_path(filelist, rpath + rootlen)) {
      struct archive *mtree = alpm_pkg_mtree_open(i->data);
      if (mtree) {
        struct archive_entry *entry;
        while (alpm_pkg_mtree_next(i->data, mtree, &entry) == ARCHIVE_OK) {
          int ret = 0;
          const char *ppath = archive_entry_pathname(entry);
          if (strncmp("./", ppath, 2) == 0) { ppath += 2; }

          if (pu_pathcmp(rpath + rootlen, ppath) != 0) { continue; }

          if (_fix_uid && fix_uid(rpath, entry) != 0) { ret = 1; }
          if (_fix_gid && fix_gid(rpath, entry) != 0) { ret = 1; }
          if (_fix_mode && fix_mode(rpath, entry) != 0) { ret = 1; }
          if (_fix_mtime && fix_mtime(rpath, entry) != 0) { ret = 1; }

          alpm_pkg_mtree_close(i->data, mtree);
          free(rpath);
          return ret;
        }
        alpm_pkg_mtree_close(i->data, mtree);
      }
    }
  }

  pu_ui_error("%s: could not find package", file);
  free(rpath);
  return 1;
}

int main(int argc, char **argv) {
  int ret = 0;

  if (!(config = parse_opts(argc, argv))) {
    ret = 1;
    goto cleanup;
  }

  if (!(handle = pu_initialize_handle_from_config(config))) {
    pu_ui_error("failed to initialize alpm");
    ret = 1;
    goto cleanup;
  }

  if (packages) {
    /* convert pkgnames to packages */
    alpm_db_t *localdb = alpm_get_localdb(handle);
    alpm_list_t *i, *pkgnames = packages;
    packages = NULL;
    for (i = pkgnames; i; i = alpm_list_next(i)) {
      alpm_pkg_t *p = alpm_db_get_pkg(localdb, i->data);
      if (p == NULL) {
        pu_ui_warn("unable to locate package '%s'", i->data);
      } else {
        ASSERT(alpm_list_append(&packages, p));
      }
    }
    alpm_list_free(pkgnames);
  } else {
    packages = alpm_list_copy(alpm_db_get_pkgcache(alpm_get_localdb(handle)));
  }

  if (packages == NULL) {
    pu_ui_error("no packages configured");
    ret = 1;
    goto cleanup;
  }

  while (optind < argc) {
    if (fix_file(argv[optind++]) != 0 ) { ret = 1; }
  }
  if (!isatty(fileno(stdin)) && errno != EBADF) {
    char *buf = NULL;
    size_t blen = 0;
    ssize_t len;
    while ((len = getline(&buf, &blen, stdin)) != -1) {
      if (buf[len - 1] == '\n') { buf[len - 1] = '\0'; }
      if (fix_file(buf) != 0) { ret = 1; }
    }
    if (!feof(stdin)) {
      pu_ui_error("unable to read from stdin (%s)", strerror(errno));
      ret = 1;
    }
    free(buf);
  }

cleanup:
  alpm_list_free(packages);
  alpm_release(handle);
  pu_config_free(config);

  return ret;
}

/* vim: set ts=2 sw=2 noet: */
