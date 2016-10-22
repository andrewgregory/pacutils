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

#ifndef PACUTILS_LOG_H
#define PACUTILS_LOG_H

typedef enum {
	PU_LOG_OPERATION_INSTALL,
	PU_LOG_OPERATION_REINSTALL,
	PU_LOG_OPERATION_UPGRADE,
	PU_LOG_OPERATION_DOWNGRADE,
	PU_LOG_OPERATION_REMOVE,
} pu_log_operation_t;

typedef struct {
	pu_log_operation_t operation;
	char *target;
	char *old_version;
	char *new_version;
} pu_log_action_t;

typedef struct {
	struct tm timestamp;
	char *caller;
	char *message;
} pu_log_entry_t;

typedef enum {
	PU_LOG_TRANSACTION_STARTED = 1,
	PU_LOG_TRANSACTION_COMPLETED,
	PU_LOG_TRANSACTION_INTERRUPTED,
	PU_LOG_TRANSACTION_FAILED,
} pu_log_transaction_status_t;

typedef struct {
	pu_log_transaction_status_t status;
	alpm_list_t *start, *end;
} pu_log_transaction_t;

typedef struct {
	FILE *stream;
	char buf[256], *next;
	int eof;
} pu_log_reader_t;

pu_log_transaction_status_t pu_log_transaction_parse(const char *message);

int pu_log_fprint_entry(FILE *stream, pu_log_entry_t *entry);
pu_log_entry_t *pu_log_reader_next(pu_log_reader_t *reader);
pu_log_reader_t *pu_log_reader_open_stream(FILE *stream);
void pu_log_reader_free(pu_log_reader_t *p);
alpm_list_t *pu_log_parse_file(FILE *stream);
void pu_log_entry_free(pu_log_entry_t *entry);

pu_log_action_t *pu_log_action_parse(char *message);
void pu_log_action_free(pu_log_action_t *action);

#endif

/* vim: set ts=2 sw=2 noet: */
