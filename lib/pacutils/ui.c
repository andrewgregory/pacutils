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
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>

#include "ui.h"
#include "util.h"
#include "../pacutils.h"

void pu_ui_vwarn(const char *fmt, va_list args) {
  fputs("warning: ", stderr);
  vfprintf(stderr, fmt, args);
  fputc('\n', stderr);
}

__attribute__((format (printf, 1, 2)))
void pu_ui_warn(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  pu_ui_vwarn(fmt, args);
  va_end(args);
}

void pu_ui_verror(const char *fmt, va_list args) {
  fputs("error: ", stderr);
  vfprintf(stderr, fmt, args);
  fputc('\n', stderr);
}

__attribute__((format (printf, 1, 2)))
void pu_ui_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  pu_ui_verror(fmt, args);
  va_end(args);
}

void pu_ui_vnotice(const char *fmt, va_list args) {
  fputs(":: ", stdout);
  vprintf(fmt, args);
  fputc('\n', stdout);
}

__attribute__((format (printf, 1, 2)))
void pu_ui_notice(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  pu_ui_vnotice(fmt, args);
  va_end(args);
}

static int64_t _pu_ui_get_time_ms(void) {
#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0) && defined(CLOCK_MONOTONIC_COARSE)
  struct timespec ts = {0, 0};
  clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
  return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
#else
  /* darwin doesn't support clock_gettime, fallback to gettimeofday */
  struct timeval tv = {0, 0};
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
#endif
}

const char *pu_ui_msg_progress(alpm_progress_t event) {
  switch (event) {
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

void pu_ui_cb_progress(void *ctx, alpm_progress_t event, const char *pkgname,
    int percent, size_t total, size_t current) {
  const char *opr = pu_ui_msg_progress(event);
  static int percent_last = -1;
  (void)ctx;

  /* don't update if nothing has changed */
  if (percent_last == percent) {
    return;
  }

  if (pkgname && pkgname[0]) {
    printf("%s %s (%zd/%zd) %d%%", opr, pkgname, current, total, percent);
  } else {
    printf("%s (%zd/%zd) %d%%", opr, current, total, percent);
  }

  if (percent == 100) {
    putchar('\n');
  } else {
    putchar('\r');
  }

  fflush(stdout);
  percent_last = percent;
}

void pu_ui_cb_event(void *ctx, alpm_event_t *event) {
  (void)ctx;
  switch (event->type) {
    case ALPM_EVENT_CHECKDEPS_START:
      puts("Checking dependencies...");
      break;
    case ALPM_EVENT_DATABASE_MISSING:
      pu_ui_warn("missing database file for '%s'",
          event->database_missing.dbname);
      break;
    case ALPM_EVENT_HOOK_RUN_START:
      if (event->hook_run.desc) {
        printf("(%zu/%zu) Running %s (%s)\n", event->hook_run.position,
            event->hook_run.total, event->hook_run.name, event->hook_run.desc);
      } else {
        printf("(%zu/%zu) Running %s\n", event->hook_run.position,
            event->hook_run.position, event->hook_run.name);
      }
      break;
    case ALPM_EVENT_HOOK_START:
      puts( event->hook.when == ALPM_HOOK_PRE_TRANSACTION
          ? "Running pre-transaction hooks..."
          : "Running post-transaction hooks...");
      break;
    case ALPM_EVENT_INTERCONFLICTS_START:
      puts("Checking package conflicts...");
      break;
    case ALPM_EVENT_KEY_DOWNLOAD_START:
      puts("Downloading keys...");
      break;
    case ALPM_EVENT_PACNEW_CREATED:
      pu_ui_notice("%s installed as %s.pacnew",
          event->pacnew_created.file, event->pacnew_created.file);
      break;
    case ALPM_EVENT_PACSAVE_CREATED:
      pu_ui_notice("%s saved as %s.pacsave",
          event->pacsave_created.file, event->pacsave_created.file);
      break;
    case ALPM_EVENT_RESOLVEDEPS_START:
      puts("Resolving dependencies...");
      break;
    case ALPM_EVENT_PKG_RETRIEVE_START:
      puts("Downloading packages...");
      break;
    case ALPM_EVENT_SCRIPTLET_INFO:
      fputs(event->scriptlet_info.line, stdout);
      break;
    case ALPM_EVENT_TRANSACTION_START:
      puts("Starting transaction...");
      break;

    /* TODO */
    case ALPM_EVENT_OPTDEP_REMOVAL:
    case ALPM_EVENT_PACKAGE_OPERATION_DONE:

    /* Ignored */
    case ALPM_EVENT_CHECKDEPS_DONE:
    case ALPM_EVENT_DB_RETRIEVE_DONE:
    case ALPM_EVENT_DB_RETRIEVE_FAILED:
    case ALPM_EVENT_DB_RETRIEVE_START:
    case ALPM_EVENT_DISKSPACE_DONE:
    case ALPM_EVENT_DISKSPACE_START:
    case ALPM_EVENT_FILECONFLICTS_DONE:
    case ALPM_EVENT_FILECONFLICTS_START:
    case ALPM_EVENT_HOOK_DONE:
    case ALPM_EVENT_HOOK_RUN_DONE:
    case ALPM_EVENT_INTEGRITY_DONE:
    case ALPM_EVENT_INTEGRITY_START:
    case ALPM_EVENT_INTERCONFLICTS_DONE:
    case ALPM_EVENT_KEYRING_DONE:
    case ALPM_EVENT_KEYRING_START:
    case ALPM_EVENT_KEY_DOWNLOAD_DONE:
    case ALPM_EVENT_LOAD_DONE:
    case ALPM_EVENT_LOAD_START:
    case ALPM_EVENT_PACKAGE_OPERATION_START:
    case ALPM_EVENT_PKG_RETRIEVE_DONE:
    case ALPM_EVENT_PKG_RETRIEVE_FAILED:
    case ALPM_EVENT_RESOLVEDEPS_DONE:
    case ALPM_EVENT_TRANSACTION_DONE:
      /* ignore */
      break;
  }
}

__attribute__((format (printf, 2, 3)))
int pu_ui_confirm(int def, const char *prompt, ...) {
  va_list args;
  va_start(args, prompt);
  fputs("\n:: ", stdout);
  vprintf(prompt, args);
  fputs(def ? " [Y/n] " : " [y/N] ", stdout);
  fflush(stdout);
  va_end(args);
  while (1) {
    int c = getchar();
    if (c == EOF) { return def; }
    if (c != '\n') { while (getchar() != '\n'); }

    switch (c) {
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

__attribute__((format (printf, 4, 5)))
long pu_ui_select_index(long def, long min, long max, const char *prompt, ...) {
  int err_sav = errno;
  va_list args;
  va_start(args, prompt);
  fputs("\n:: ", stdout);
  vprintf(prompt, args);
  printf(" [%ld] ", def);
  fflush(stdout);
  va_end(args);

  while (1) {
    char input[32];

    if (fgets(input, 32, stdin)) {
      size_t len = strlen(input);
      char *endptr;
      long selected;

      if (len == 0 || (len == 1 && input[0] == '\n')) {
        errno = err_sav;
        return def;
      } else if (input[len - 1] == '\n') {
        input[len - 1] = '\0';
      }

      errno = 0;
      selected = strtol(input, &endptr, 10);
      if (errno != 0 || selected > max || selected < min || *endptr != '\0') {
        printf("\n:: Invalid input '%s', please enter a number in the range %ld-%ld: ",
            input, min, max);
        continue;
      }

      errno = err_sav;
      return selected;
    } else if (errno != EINTR) {
      errno = err_sav;
      return def;
    }
  }
}

void pu_ui_cb_question(void *ctx, alpm_question_t *question) {
  (void)ctx;
  switch (question->type) {
    case ALPM_QUESTION_INSTALL_IGNOREPKG: {
      alpm_question_install_ignorepkg_t *q = &question->install_ignorepkg;
      q->install = pu_ui_confirm(1,
              "Install ignored package '%s'?", alpm_pkg_get_name(q->pkg));
    }
    break;
    case ALPM_QUESTION_REPLACE_PKG: {
      alpm_question_replace_t *q = &question->replace;
      q->replace = pu_ui_confirm(1, "Replace '%s' with '%s'?",
              alpm_pkg_get_name(q->oldpkg), alpm_pkg_get_name(q->newpkg));
    }
    break;
    case ALPM_QUESTION_CONFLICT_PKG: {
      alpm_question_conflict_t *q = (alpm_question_conflict_t *) question;
      alpm_conflict_t *c = q->conflict;
      q->remove = pu_ui_confirm(1, "'%s' conflicts with '%s'.  Remove '%s'?",
              c->package1, c->package2, c->package2);
    }
    break;
    case ALPM_QUESTION_REMOVE_PKGS: {
      alpm_question_remove_pkgs_t *q = &question->remove_pkgs;
      alpm_list_t *i;
      pu_ui_notice("The following packages have unresolvable dependencies:");
      for (i = q->packages; i; i = i->next) {
        fputs("  ", stdout);
        pu_fprint_pkgspec(stdout, i->data);
      }
      q->skip = pu_ui_confirm(0,
              "Remove the above packages from the transaction?");
    }
    break;
    case ALPM_QUESTION_SELECT_PROVIDER: {
      alpm_question_select_provider_t *q = &question->select_provider;
      alpm_list_t *i;
      alpm_depend_t *dep = q->depend;
      int count = 0;

      pu_ui_notice("There are multiple providers for the following dependency:");
      printf("  %s", dep->name);
      switch (dep->mod) {
        case ALPM_DEP_MOD_ANY:
          break;
        case ALPM_DEP_MOD_EQ:
          printf("=%s", dep->version);
          break;
        case ALPM_DEP_MOD_GE:
          printf(">=%s", dep->version);
          break;
        case ALPM_DEP_MOD_LE:
          printf("<=%s", dep->version);
          break;
        case ALPM_DEP_MOD_GT:
          printf(">%s", dep->version);
          break;
        case ALPM_DEP_MOD_LT:
          printf("<%s", dep->version);
          break;
      }
      fputs("\n\n", stdout);

      for (i = q->providers; i; i = i->next) {
        printf("  %d - ", ++count);
        pu_fprint_pkgspec(stdout, i->data);
        fputs("\n", stdout);
      }

      q->use_index = pu_ui_select_index(count ? 1 : 0, 0, count,
              "Select a provider (0 to skip)") - 1;
    }
    break;
    case ALPM_QUESTION_CORRUPTED_PKG: {
      alpm_question_corrupted_t *q = &question->corrupted;
      q->remove = pu_ui_confirm(1, "Delete corrupted file '%s' (%s)",
              q->filepath, alpm_strerror(q->reason));
    }
    break;
    case ALPM_QUESTION_IMPORT_KEY: {
      alpm_question_import_key_t *q = &question->import_key;
      alpm_pgpkey_t *key = q->key;
      char created[12];
      time_t time = (time_t) key->created;

      if (strftime(created, 12, "%Y-%m-%d", localtime(&time)) == 0) {
        strcpy(created, "(unknown)");
      }

      q->import = pu_ui_confirm(1,
              (key->revoked
                  ? "Import PGP key %u%c/%s, '%s', created: %s (revoked)"
                  : "Import PGP key %u%c/%s, '%s', created: %s"),
              key->length, key->pubkey_algo, key->fingerprint, key->uid, created);
    }
    break;
  }
}

typedef struct _pu_ui_download_status_t {
  char *filename;
  int optional;
  off_t initial_size; /* for resuming partial downloads */
  off_t downloaded;
  off_t total;
} _pu_ui_download_status_t;

static void _pu_ui_clear_line(FILE *out) {
  fputs("\x1B[K", out);
}

void pu_ui_cb_download(void *ctx, const char *filename,
    alpm_download_event_type_t event, void *data) {
  static pu_ui_ctx_download_t default_ctx = {
    .update_interval_same = 200,
    .update_interval_next = 1000
  };
  pu_ui_ctx_download_t *c = ctx ? ctx : &default_ctx;

  int64_t now;
  int should_update = 0;

  if (c->out == NULL) { c->out = stdout; }

  switch (event) {
    case ALPM_DOWNLOAD_INIT: {
      _pu_ui_download_status_t *s = calloc(sizeof(_pu_ui_download_status_t), 1);
      alpm_list_append(&c->active_downloads, s);
      s->filename = strdup(filename);
      s->optional = ((alpm_download_event_init_t *)data)->optional;
      break;
    }
    case ALPM_DOWNLOAD_PROGRESS: {
      for (alpm_list_t *i = c->active_downloads; i; i = i->next) {
        _pu_ui_download_status_t *s = i->data;
        if (strcmp(s->filename, filename) == 0) {
          alpm_download_event_progress_t *d = data;
          s->downloaded = d->downloaded;
          s->total      = d->total;
          break;
        }
      }
      break;
    }
    case ALPM_DOWNLOAD_RETRY: {
      alpm_download_event_retry_t *r = data;
      for (alpm_list_t *i = c->active_downloads; i; i = i->next) {
        _pu_ui_download_status_t *s = i->data;
        if (strcmp(s->filename, filename) == 0) {
          if (r->resume) {
            s->initial_size = s->downloaded;
          } else {
            s->downloaded = 0;
          }
          break;
        }
      }
      break;
    }
    case ALPM_DOWNLOAD_COMPLETED: {
      alpm_download_event_completed_t *d = data;
      int optional = 0;

      should_update = 1;

      for (alpm_list_t *i = c->active_downloads; i; i = i->next) {
        _pu_ui_download_status_t *s = i->data;
        if (strcmp(s->filename, filename) == 0) {
          optional = s->optional;
          c->active_downloads = alpm_list_remove_item(c->active_downloads, i);
          free(s->filename);
          free(s);
          free(i);
          break;
        }
      }

      switch (d->result) {
        case 0:
          _pu_ui_clear_line(c->out);
          fprintf(c->out, "%s (%jd/%jd) 100%%\n", filename,
              (intmax_t)d->total, (intmax_t)d->total);
          break;
        case 1:
          _pu_ui_clear_line(c->out);
          fprintf(c->out, "%s is up to date\n", filename);
          break;
        default:
          if (!optional) {
            _pu_ui_clear_line(c->out);
            fprintf(c->out, "%s failed to download\n", filename);
          }
          break;
      }

      break;
    }
  }

  now = _pu_ui_get_time_ms();
  if (should_update || now - c->last_advance >= c->update_interval_next) {
    c->index++;
    should_update = 1;
    c->last_update = c->last_advance = now;
  } else if (now - c->last_update >= c->update_interval_same) {
    should_update = 1;
    c->last_update = now;
  }

  if (should_update) {
    int num = alpm_list_count(c->active_downloads);
    alpm_list_t *n;
    if (c->index >= num) { c->index = 0; }
    if ((n = alpm_list_nth(c->active_downloads, c->index))) {
      _pu_ui_download_status_t *s = n->data;
      intmax_t downloaded = s->initial_size + s->downloaded;
      _pu_ui_clear_line(c->out);
      if (s->total) {
        fprintf(c->out, "(%d/%d) %s (%jd/%jd) %d%%\r",
            c->index + 1, num, s->filename, downloaded, (intmax_t)s->total,
            (int)(100 * downloaded / s->total));
      } else {
        fprintf(c->out, "(%d/%d) %s (%jd)\r",
            c->index + 1, num, s->filename, (intmax_t)s->downloaded);
      }
    }
    fflush(c->out);
  }
}

void pu_ui_display_transaction(alpm_handle_t *handle) {
  off_t install = 0, download = 0, delta = 0;
  char size[20];
  alpm_db_t *ldb = alpm_get_localdb(handle);
  alpm_list_t *i;

  for (i = alpm_trans_get_remove(handle); i; i = i->next) {
    alpm_pkg_t *p = i->data;
    printf("removing %s/%s (%s)\n",
        alpm_db_get_name(alpm_pkg_get_db(p)),
        alpm_pkg_get_name(p),
        alpm_pkg_get_version(p));

    install -= alpm_pkg_get_isize(i->data);
    delta -= alpm_pkg_get_isize(i->data);
  }

  for (i = alpm_trans_get_add(handle); i; i = i->next) {
    alpm_pkg_t *p = i->data;
    alpm_pkg_t *lpkg = alpm_db_get_pkg(ldb, alpm_pkg_get_name(p));

    switch (alpm_pkg_get_origin(p)) {
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

    if (lpkg) {
      printf(" (%s -> %s)\n",
          alpm_pkg_get_version(lpkg), alpm_pkg_get_version(p));
    } else {
      printf(" (%s)\n", alpm_pkg_get_version(p));
    }

    install  += alpm_pkg_get_isize(p);
    download += alpm_pkg_download_size(p);
    delta    += alpm_pkg_get_isize(p);
    if (lpkg) {
      delta  -= alpm_pkg_get_isize(lpkg);
    }
  }

  fputs("\n", stdout);
  printf("Download Size:  %10s\n", pu_hr_size(download, size));
  printf("Installed Size: %10s\n", pu_hr_size(install, size));
  printf("Size Delta:     %10s\n", pu_hr_size(delta, size));
}

pu_config_t *pu_ui_config_parse_sysroot(pu_config_t *dest,
    const char *file, const char *root) {
  pu_config_t *config = pu_config_new();
  pu_config_reader_t *reader = pu_config_reader_new_sysroot(config, file, root);

  if (config == NULL || reader == NULL) {
    pu_ui_error("reading '%s' failed (%s)", file, strerror(errno));
    pu_config_free(config);
    pu_config_reader_free(reader);
    return NULL;
  }

  while (pu_config_reader_next(reader) != -1) {
    switch (reader->status) {
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
  if (reader->error) {
    if (!reader->eof) {
      pu_ui_error("reading '%s' failed (%s)", reader->file, strerror(errno));
    }
    pu_config_reader_free(reader);
    pu_config_free(config);
    return NULL;
  }
  pu_config_reader_free(reader);

  if (dest) {
    pu_config_merge(dest, config);
    config = NULL;
  } else {
    dest = config;
  }

  return dest;
}

pu_config_t *pu_ui_config_parse(pu_config_t *dest, const char *file) {
  return pu_ui_config_parse_sysroot(dest, file, "/");
}

pu_config_t *pu_ui_config_load_sysroot(pu_config_t *dest,
    const char *file, const char *root) {
  int allocd = dest == NULL ? 1 : 0;
  if ((dest = pu_ui_config_parse_sysroot(dest, file, root)) == NULL) { return NULL; }

  if (pu_config_resolve_sysroot(dest, root) != 0) {
    pu_ui_error("resolving config values failed (%s)", strerror(errno));
    if (allocd) { pu_config_free(dest); }
    return NULL;
  }

  return dest;
}

pu_config_t *pu_ui_config_load(pu_config_t *dest, const char *file) {
  return pu_ui_config_load_sysroot(dest, file, "/");
}

/* vim: set ts=2 sw=2 et: */
