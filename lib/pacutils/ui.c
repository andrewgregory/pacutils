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

#include <errno.h>
#include <string.h>
#include <sys/time.h>

#include "ui.h"
#include "util.h"

#define PU_MAX_REFRESH_MS 200

__attribute__((format (printf, 1, 2)))
void pu_ui_warn(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  fputs("warning: ", stderr);
  vfprintf(stderr, fmt, args);
  fputc('\n', stderr);
  va_end(args);
}

__attribute__((format (printf, 1, 2)))
void pu_ui_error(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  fputs("error: ", stderr);
  vfprintf(stderr, fmt, args);
  fputc('\n', stderr);
  va_end(args);
}

static long _pu_ui_time_diff(struct timeval *t1, struct timeval *t2)
{
  return (t1->tv_sec - t2->tv_sec) * 1000 + (t1->tv_usec - t2->tv_usec) / 1000;
}

const char *pu_ui_msg_progress(alpm_progress_t event)
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

void pu_ui_cb_progress(alpm_progress_t event, const char *pkgname, int percent,
    size_t total, size_t current)
{
  const char *opr = pu_ui_msg_progress(event);
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

__attribute__((format (printf, 2, 3)))
int pu_ui_confirm(int def, const char *prompt, ...)
{
  va_list args;
  va_start(args, prompt);
  fputs("\n:: ", stdout);
  vprintf(prompt, args);
  fputs(def ? " [Y/n]" : " [y/N]", stdout);
  fflush(stdout);
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

void pu_ui_cb_question(alpm_question_t *question)
{
  switch(question->type) {
    case ALPM_QUESTION_INSTALL_IGNOREPKG:
      {
        alpm_question_install_ignorepkg_t *q = &question->install_ignorepkg;
        q->install = pu_ui_confirm(1,
            "Install ignored package '%s'?", alpm_pkg_get_name(q->pkg));
      }
      break;
    case ALPM_QUESTION_REPLACE_PKG:
      {
        alpm_question_replace_t *q = &question->replace;
        q->replace = pu_ui_confirm(1, "Replace '%s' with '%s'?",
            alpm_pkg_get_name(q->oldpkg), alpm_pkg_get_name(q->newpkg));
      }
      break;
    case ALPM_QUESTION_CONFLICT_PKG:
      {
        alpm_question_conflict_t *q = (alpm_question_conflict_t*) question;
        alpm_conflict_t *c = q->conflict;
        q->remove = pu_ui_confirm(1, "'%s' conflicts with '%s'.  Remove '%s'?",
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

void pu_ui_cb_download(const char *filename, off_t xfered, off_t total)
{
  static struct timeval last_update = {0, 0};
  int percent;

  if(xfered > 0 && xfered < total) {
    struct timeval now;
    gettimeofday(&now, NULL);
    if(_pu_ui_time_diff(&now, &last_update) < PU_MAX_REFRESH_MS) {
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

void pu_ui_display_transaction(alpm_handle_t *handle)
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

pu_config_t *pu_ui_config_parse(pu_config_t *dest, const char *file) {
  pu_config_t *config = pu_config_new();
  pu_config_reader_t *reader = pu_config_reader_new(config, file);

  if(config == NULL || reader == NULL) {
    pu_ui_error("reading '%s' failed (%s)", file, strerror(errno));
    pu_config_free(config);
    pu_config_reader_free(reader);
    return NULL;
  }

  while(pu_config_reader_next(reader) != -1) {
    switch(reader->status) {
      case PU_CONFIG_READER_STATUS_INVALID_VALUE:
        pu_ui_error("config %s line %d: invalid value '%s' for '%s'",
            reader->file, reader->line, reader->value, reader->key);
        break;
      case PU_CONFIG_READER_STATUS_UNKNOWN_OPTION:
        pu_ui_warn("config %s line %d: unknown option '%s'",
            reader->file, reader->line, reader->key);
        break;
      case PU_CONFIG_READER_STATUS_OK:
        /* todo debugging */
        break;
      case PU_CONFIG_READER_STATUS_ERROR:
        /* should never get here, hard errors return -1 */
        break;
    }
  }
  if(reader->error) {
    if(!reader->eof) {
      pu_ui_error("reading '%s' failed (%s)", reader->file, strerror(errno));
    }
    pu_config_reader_free(reader);
    pu_config_free(config);
    return NULL;
  }
  pu_config_reader_free(reader);

  if(dest) {
    pu_config_merge(dest, config);
    config = NULL;
  } else {
    dest = config;
  }

  return dest;
}

pu_config_t *pu_ui_config_load(pu_config_t *dest, const char *file) {
  int allocd = dest == NULL ? 1 : 0;
  if((dest = pu_ui_config_parse(dest, file)) == NULL) { return NULL; }

  if(pu_config_resolve(dest) != 0) {
    pu_ui_error("resolving config values failed (%s)", strerror(errno));
    if(allocd) { pu_config_free(dest); }
    return NULL;
  }

  return dest;
}

/* vim: set ts=2 sw=2 et: */
