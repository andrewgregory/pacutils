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

#ifndef PACUTILS_HOOK_H
#define PACUTILS_HOOK_H

typedef enum pu_hook_when_t {
  PU_HOOK_PRE_TRANSACTION,
  PU_HOOK_POST_TRANSACTION,
} pu_hook_when_t;

typedef enum pu_hook_trigger_type_t {
  PU_HOOK_TRIGGER_TYPE_PACKAGE,
  PU_HOOK_TRIGGER_TYPE_FILE,
} pu_hook_trigger_type_t;

typedef enum pu_hook_trigger_operation_t {
  PU_HOOK_TRIGGER_OPERATION_INSTALL = (1 << 0),
  PU_HOOK_TRIGGER_OPERATION_UPGRADE = (1 << 1),
  PU_HOOK_TRIGGER_OPERATION_REMOVE = (1 << 2),
} pu_hook_trigger_operation_t;

typedef struct pu_hook_trigger_t {
  enum pu_hook_trigger_operation_t op;
  enum pu_hook_trigger_type_t type;
  alpm_list_t *targets;
} pu_hook_trigger_t;

typedef struct pu_hook_t {
  char *name, *desc, **cmd;
  pu_hook_when_t when;
  alpm_list_t *triggers, *depends;
  int abort_on_fail, needs_targets;
} pu_hook_t;

typedef enum pu_hook_reader_status_t {
  PU_HOOK_READER_STATUS_OK,
  PU_HOOK_READER_STATUS_ERROR,
  PU_HOOK_READER_STATUS_INVALID_VALUE,
  PU_HOOK_READER_STATUS_INVALID_SECTION,
  PU_HOOK_READER_STATUS_INVALID_OPTION,
} pu_hook_reader_status_t;

typedef struct pu_hook_reader_t {
  int eof, line, error;
  char *file, *section, *key, *value;
  pu_hook_t *hook;
  pu_hook_reader_status_t status;

  void *_mini;
} pu_hook_reader_t;

pu_hook_reader_t *pu_hook_reader_finit(pu_hook_t *hook, FILE *stream);
pu_hook_reader_t *pu_hook_reader_init(pu_hook_t *hook, const char *file);
int pu_hook_reader_next(pu_hook_reader_t *reader);
void pu_hook_reader_free(pu_hook_reader_t *reader);

pu_hook_t *pu_hook_new(void);
void pu_hook_free(pu_hook_t *hook);

#endif /* PACUTILS_CONFIG_H */

/* vim: set ts=2 sw=2 et: */
