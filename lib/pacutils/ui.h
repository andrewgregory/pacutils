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

#include "config.h"

#ifndef PACUTILS_UI_H
#define PACUTILS_UI_H

typedef struct pu_ui_ctx_download_t {
  /* settings */

  /* FILE to write to */
  FILE *out;
  /* how frequently to update the current download (ms),
   * setting too low a value can cause flickering */
  uint64_t update_interval_same;
  /* how frequently to advance to the next download (ms),
   * setting too low a value can cause flickering */
  uint64_t update_interval_next;

  /* context */
  uint64_t last_update;
  uint64_t last_advance;
  alpm_list_t *active_downloads;
  int index;
} pu_ui_ctx_download_t;

void pu_ui_error(const char *fmt, ...);
void pu_ui_warn(const char *fmt, ...);
void pu_ui_notice(const char *fmt, ...);

void pu_ui_verror(const char *fmt, va_list args);
void pu_ui_vwarn(const char *fmt, va_list args);
void pu_ui_vnotice(const char *fmt, va_list args);

int pu_ui_confirm(int def, const char *prompt, ...);
long pu_ui_select_index(long def, long min, long max, const char *prompt, ...);

const char *pu_ui_msg_progress(alpm_progress_t event);

void pu_ui_display_transaction(alpm_handle_t *handle);

void pu_ui_cb_download(void *ctx, const char *filename,
    alpm_download_event_type_t event, void *data);
void pu_ui_cb_progress(void *ctx, alpm_progress_t event, const char *pkgname,
    int percent, size_t total, size_t current);
void pu_ui_cb_question(void *ctx, alpm_question_t *question);
void pu_ui_cb_event(void *ctx, alpm_event_t *event);

pu_config_t *pu_ui_config_parse(pu_config_t *dest, const char *file);
pu_config_t *pu_ui_config_load(pu_config_t *dest, const char *file);

pu_config_t *pu_ui_config_parse_sysroot(pu_config_t *dest, const char *file,
    const char *root);
pu_config_t *pu_ui_config_load_sysroot(pu_config_t *dest, const char *file,
    const char *root);

#endif /* PACUTILS_UI_H */
