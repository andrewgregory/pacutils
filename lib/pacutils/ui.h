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

void pu_ui_error(const char *fmt, ...);
void pu_ui_warn(const char *fmt, ...);

int pu_ui_confirm(int def, const char *prompt, ...);

const char *pu_ui_msg_progress(alpm_progress_t event);

void pu_ui_display_transaction(alpm_handle_t *handle);

void pu_ui_cb_download(const char *filename, off_t xfered, off_t total);
void pu_ui_cb_progress(alpm_progress_t event, const char *pkgname, int percent,
    size_t total, size_t current);
void pu_ui_cb_question(alpm_question_t *question);
void pu_ui_cb_event(alpm_event_t *event);

pu_config_t *pu_ui_config_parse(pu_config_t *dest, const char *file);
pu_config_t *pu_ui_config_load(pu_config_t *dest, const char *file);

pu_config_t *pu_ui_config_parse_sysroot(pu_config_t *dest, const char *file, const char *root);
pu_config_t *pu_ui_config_load_sysroot(pu_config_t *dest, const char *file, const char *root);

#endif /* PACUTILS_UI_H */

/* vim: set ts=2 sw=2 et: */
