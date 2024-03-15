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

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>

#include <pacutils.h>

#include "config-defaults.h"

const char *myname = "pacfile", *myver = BUILDVER;

int checkfs = 0;
alpm_list_t *pkgnames = NULL;
const char *sysroot = NULL;

enum longopt_flags {
  FLAG_CONFIG = 1000,
  FLAG_DBPATH,
  FLAG_HELP,
  FLAG_PACKAGE,
  FLAG_ROOT,
  FLAG_SYSROOT,
  FLAG_VERSION,
};

void usage(int ret) {
  FILE *stream = (ret ? stderr : stdout);
#define hputs(s) fputs(s"\n", stream)
  hputs("pacfile - display information about pacman-managed files");
  hputs("usage:  pacfile [options] <file>...");
  hputs("        pacfile (--help|--version)");
  hputs("options:");
  hputs("   --config=<path>    set an alternate configuration file");
  hputs("   --dbpath=<path>    set an alternate database location");
  hputs("   --root=<path>      set an alternate installation root");
  hputs("   --sysroot=<path>   set an alternate system root");
  hputs("   --help             display this help information");
  hputs("   --version          display version information");
  hputs("   --package=<pkg>    limit information to specified package(s)");
  hputs("   --check            compare database values to filesystem");
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
    { "help", no_argument, NULL, FLAG_HELP         },
    { "root", required_argument, NULL, FLAG_ROOT         },
    { "sysroot", required_argument, NULL, FLAG_SYSROOT      },
    { "version", no_argument, NULL, FLAG_VERSION      },
    { "no-check", no_argument, &checkfs, 0                 },
    { "check", no_argument, &checkfs, 1                 },
    { "package", required_argument, NULL, FLAG_PACKAGE      },
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
      case FLAG_DBPATH:
        free(config->dbpath);
        config->dbpath = strdup(optarg);
        break;
      case FLAG_HELP:
        usage(0);
        break;
      case FLAG_ROOT:
        free(config->rootdir);
        config->rootdir = strdup(optarg);
        break;
      case FLAG_SYSROOT:
        sysroot = optarg;
        break;
      case FLAG_VERSION:
        pu_print_version(myname, myver);
        exit(0);
        break;
      case FLAG_PACKAGE:
        pkgnames = alpm_list_add(pkgnames, strdup(optarg));
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

mode_t cmp_mode(struct archive_entry *entry, struct stat *st) {
  mode_t mask = 07777;
  mode_t pmode = archive_entry_mode(entry);
  mode_t perm = pmode & mask;
  const char *type = mode_str(pmode);


  printf("mode:   %o", perm);
  if (st && perm != (st->st_mode & mask)) {
    printf(" (%o on filesystem)", st->st_mode & mask);
  }
  putchar('\n');

  printf("type:   %s", type);
  if (st) {
    const char *ftype = mode_str(st->st_mode);
    if (strcmp(type, ftype) != 0) {
      printf(" (%s on filesystem)", ftype);
    }
  }
  putchar('\n');

  return pmode;
}

void cmp_mtime(struct archive_entry *entry, struct stat *st) {
  struct tm ltime;
  char time_buf[26];

  time_t t = archive_entry_mtime(entry);
  strftime(time_buf, 26, "%F %T", localtime_r(&t, &ltime));
  printf("mtime:  %s", time_buf);

  if (st && t != st->st_mtime) {
    strftime(time_buf, 26, "%F %T", localtime_r(&st->st_mtime, &ltime));
    printf(" (%s on filesystem)", time_buf);
  }

  putchar('\n');
}

void cmp_target(struct archive_entry *entry, struct stat *st,
    const char *path) {
  const char *ptarget = archive_entry_symlink(entry);
  printf("target: %s", ptarget);

  if (st) {
    char ftarget[PATH_MAX];
    ssize_t len = readlink(path, ftarget, PATH_MAX);
    ftarget[len] = '\0';
    if (strcmp(ptarget, ftarget) != 0) {
      printf(" (%s on filesystem)", ftarget);
    }
  }

  putchar('\n');
}

void cmp_uid(struct archive_entry *entry, struct stat *st) {
  uid_t puid = archive_entry_uid(entry);
  struct passwd *pw = getpwuid(puid);

  printf("owner:  %d/%s", puid, pw ? pw->pw_name : "unknown user");

  if (st && puid != st->st_uid) {
    pw = getpwuid(st->st_uid);
    printf(" (%d/%s on filesystem)",
        st->st_uid, pw ? pw->pw_name : "unknown user");
  }

  putchar('\n');
}

void cmp_gid(struct archive_entry *entry, struct stat *st) {
  gid_t pgid = archive_entry_gid(entry);
  struct group *gr = getgrgid(pgid);

  printf("group:  %d/%s", pgid, gr ? gr->gr_name : "unknown group");

  if (st && pgid != st->st_gid) {
    gr = getgrgid(st->st_gid);
    printf(" (%d/%s on filesystem)",
        st->st_gid, gr ? gr->gr_name : "unknown group");
  }

  putchar('\n');
}

void cmp_size(struct archive_entry *entry, struct stat *st) {
  int64_t psize = archive_entry_size(entry);
  char hr_size[20];

  printf("size:   %s", pu_hr_size(psize, hr_size));

  if (st && psize != st->st_size) {
    printf(" (%s on filesystem)", pu_hr_size(st->st_size, hr_size));
  }

  putchar('\n');
}

void cmp_sha256sum(struct stat *st,
    alpm_handle_t *handle, alpm_pkg_t *pkg, const char *path) {
  pu_mtree_reader_t *reader;
  pu_mtree_t *m;

  if ((reader = pu_mtree_reader_open_package(handle, pkg)) == NULL) {
    pu_ui_warn("%s: mtree data not available", alpm_pkg_get_name(pkg));
    return;
  }

  while ((m = pu_mtree_reader_next(reader, NULL))) {
    char *sha = NULL;
    if (strcmp(m->path, path) != 0) { pu_mtree_free(m); continue; }

    if (checkfs && S_ISREG(st->st_mode)) {
      char rpath[PATH_MAX];
      snprintf(rpath, PATH_MAX, "%s%s", alpm_option_get_root(handle), path);
      if ((sha = alpm_compute_sha256sum(rpath)) == NULL) {
        pu_ui_warn("%s: '%s' read error (%s)",
            alpm_pkg_get_name(pkg), rpath, strerror(errno));
      }
    }

    printf("sha256: %s", m->sha256digest);

    if (sha) {
      if (strcmp(sha, m->sha256digest) != 0) {
        printf(" (%s on filesystem)", sha);
      }
      free(sha);
    }
    putchar('\n');

    pu_mtree_reader_free(reader);
    pu_mtree_free(m);

    return;
  }

  if (!reader->eof) {
    pu_ui_warn("%s: error reading mtree data (%s)",
        alpm_pkg_get_name(pkg), strerror(errno));
  } else {
    pu_ui_warn("%s: error '%s' not found in mtree data",
        alpm_pkg_get_name(pkg), path);
  }
  pu_mtree_reader_free(reader);
}

void cmp_md5sum(struct stat *st,
    alpm_handle_t *handle, alpm_pkg_t *pkg, const char *path) {
  pu_mtree_reader_t *reader;
  pu_mtree_t *m;

  if ((reader = pu_mtree_reader_open_package(handle, pkg)) == NULL) {
    pu_ui_warn("%s: mtree data not available", alpm_pkg_get_name(pkg));
    return;
  }

  while ((m = pu_mtree_reader_next(reader, NULL))) {
    char *md5 = NULL;
    if (strcmp(m->path, path) != 0) { pu_mtree_free(m); continue; }

    if (checkfs && S_ISREG(st->st_mode)) {
      char rpath[PATH_MAX];
      snprintf(rpath, PATH_MAX, "%s%s", alpm_option_get_root(handle), path);
      if ((md5 = alpm_compute_md5sum(rpath)) == NULL) {
        pu_ui_warn("%s: '%s' read error (%s)",
            alpm_pkg_get_name(pkg), rpath, strerror(errno));
      }
    }

    printf("md5sum: %s", m->md5digest);

    if (md5) {
      if (strcmp(md5, m->md5digest) != 0) {
        printf(" (%s on filesystem)", md5);
      }
      free(md5);
    }
    putchar('\n');

    pu_mtree_reader_free(reader);
    pu_mtree_free(m);

    return;
  }

  if (!reader->eof) {
    pu_ui_warn("%s: error reading mtree data (%s)",
        alpm_pkg_get_name(pkg), strerror(errno));
  } else {
    pu_ui_warn("%s: error '%s' not found in mtree data",
        alpm_pkg_get_name(pkg), path);
  }
  pu_mtree_reader_free(reader);
}

int main(int argc, char **argv) {
  pu_config_t *config = NULL;
  alpm_handle_t *handle = NULL;
  alpm_list_t *pkgs = NULL;
  int ret = 0;
  size_t rootlen;
  const char *root;

  if (!(config = parse_opts(argc, argv))) {
    goto cleanup;
  }

  if (!(handle = pu_initialize_handle_from_config(config))) {
    fprintf(stderr, "error: failed to initialize alpm.\n");
    ret = 1;
    goto cleanup;
  }

  root = alpm_option_get_root(handle);
  rootlen = strlen(root);

  if (pkgnames) {
    alpm_list_t *p;
    alpm_db_t *db = alpm_get_localdb(handle);
    for (p = pkgnames; p; p = alpm_list_next(p)) {
      alpm_pkg_t *pkg = alpm_db_get_pkg(db, p->data);
      if (pkg) {
        pkgs = alpm_list_add(pkgs, pkg);
      } else {
        fprintf(stderr, "error: could not locate package '%s'\n",
            (char *) p->data);
        ret = 1;
      }
    }
  } else {
    pkgs = alpm_list_copy(alpm_db_get_pkgcache(alpm_get_localdb(handle)));
  }

  while (optind < argc) {
    const char *relfname, *filename = argv[optind];

    int found = 0;
    alpm_list_t *p;

    if (strncmp(filename, root, rootlen) == 0) {
      relfname = filename + rootlen;
    } else {
      relfname = filename;
    }

    for (p = pkgs; p; p = alpm_list_next(p)) {
      alpm_file_t *pfile = pu_filelist_contains_path(
              alpm_pkg_get_files(p->data), relfname);
      if (pfile) {
        alpm_list_t *b;
        char full_path[PATH_MAX];
        snprintf(full_path, PATH_MAX, "%s%s", root, pfile->name);

        if (found++) { putchar('\n'); }

        printf("file:   %s\n", pfile->name);
        printf("owner:  %s\n", alpm_pkg_get_name(p->data));

        /* backup file status */
        for (b = alpm_pkg_get_backup(p->data); b; b = b->next) {
          alpm_backup_t *bak = b->data;
          if (pu_pathcmp(relfname, bak->name) == 0) {
            fputs("backup: yes\n", stdout);
            fprintf(stdout, "md5sum: %s", bak->hash);

            if (checkfs) {
              char *md5sum = alpm_compute_md5sum(full_path);
              if (!md5sum) {
                fprintf(stderr, "warning: could not calculate md5sum for '%s'\n",
                    full_path);
                ret = 1;
              } else {
                if (strcmp(md5sum, bak->hash) != 0) {
                  fprintf(stdout, " (%s on filesystem)", md5sum);
                }
                free(md5sum);
              }
            }

            putchar('\n');
            break;
          }
        }
        if (!b) {
          fputs("backup: no\n", stdout);
        }

        /* MTREE info */
        struct archive *mtree = alpm_pkg_mtree_open(p->data);
        if (mtree) {

          struct archive_entry *entry;
          while (alpm_pkg_mtree_next(p->data, mtree, &entry) == ARCHIVE_OK) {
            struct stat sbuf, *st = NULL;

            const char *ppath = archive_entry_pathname(entry);
            if (strncmp("./", ppath, 2) == 0) { ppath += 2; }

            if (pu_pathcmp(relfname, ppath) != 0) { continue; }

            if (checkfs) {
              if (lstat(full_path, &sbuf) != 0) {
                fprintf(stderr, "warning: could not stat '%s' (%s)\n",
                    full_path, strerror(errno));
                ret = 1;
              } else {
                st = &sbuf;
              }
            }

            if (S_ISLNK(cmp_mode(entry, st))) {
              cmp_target(entry, st, full_path);
            }
            cmp_mtime(entry, st);
            cmp_uid(entry, st);
            cmp_gid(entry, st);

            if (archive_entry_filetype(entry) == AE_IFREG) {
              cmp_size(entry, st);
              cmp_sha256sum(st, handle, p->data, ppath);
              cmp_md5sum(st, handle, p->data, ppath);
            }

            break;
          }
          alpm_pkg_mtree_close(p->data, mtree);
        }
      }
    }

    if (!found) {
      printf("no package owns '%s'\n", filename);
    }

    ++optind;
    if (optind < argc) {
      fputs("\n", stdout);
    }
  }

cleanup:
  alpm_release(handle);
  pu_config_free(config);
  alpm_list_free(pkgs);
  FREELIST(pkgnames);

  return ret;
}
