#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <alpm_list.h>

#include "log.h"

void pu_log_action_free(pu_log_action_t *action)
{
	if(!action) return;
	free(action->target);
	free(action->old_version);
	free(action->new_version);
	free(action);
}

pu_log_action_t *pu_log_action_parse(char *message)
{
	char operation[20], *version = NULL;
	pu_log_action_t *a = calloc(sizeof(pu_log_action_t), 1);

	if(sscanf(message, "%20s %ms (%m[^)])", operation, &a->target, &version) == 3) {
		if(strcmp(operation, "installed") == 0) {
			a->operation = PU_LOG_OPERATION_INSTALL;
			a->new_version = version;
		} else if(strcmp(operation, "reinstalled") == 0) {
			a->operation = PU_LOG_OPERATION_REINSTALL;
			a->new_version = version;
		} else if(strcmp(operation, "upgraded") == 0) {
			a->operation = PU_LOG_OPERATION_UPGRADE;
			if(sscanf(version, "%ms -> %ms", &a->old_version, &a->new_version) == 2) {
				free(version);
			} else {
				goto error;
			}
		} else if(strcmp(operation, "downgraded") == 0) {
			a->operation = PU_LOG_OPERATION_DOWNGRADE;
			if(sscanf(version, "%ms -> %ms", &a->old_version, &a->new_version) == 2) {
				free(version);
			} else {
				goto error;
			}
		} else if(strcmp(operation, "removed") == 0) {
			a->operation = PU_LOG_OPERATION_REMOVE;
			a->old_version = version;
		} else {
			goto error;
		}
	} else {
		goto error;
	}

	return a;

error:
	free(version);
	pu_log_action_free(a);
	return NULL;
}

int pu_log_fprint_entry(FILE *stream, pu_log_entry_t *entry)
{
	char timestamp[50];

	strftime(timestamp, 50, "%F %R", &entry->timestamp);
	if(entry->caller) {
		return printf("[%s] [%s] %s", timestamp, entry->caller, entry->message);
	} else {
		return printf("[%s] %s", timestamp, entry->message);
	}
}

pu_log_parser_t *pu_log_parser_new(FILE *stream) {
	pu_log_parser_t *parser = calloc(sizeof(pu_log_parser_t), 1);
	parser->stream = stream;
	return parser;
}

pu_log_entry_t *pu_log_parser_next(pu_log_parser_t *parser) {
	char *p;
	pu_log_entry_t *entry = calloc(sizeof(pu_log_entry_t), 1);

	if(entry == NULL) { errno = ENOMEM; return NULL; }

	if(parser->next == NULL && fgets(parser->buf, 256, parser->stream) == NULL) {
		parser->eof = feof(parser->stream);
		free(entry);
		return NULL;
	}

	if(!(p = strptime(parser->buf, "[%Y-%m-%d %H:%M]", &entry->timestamp))) {
		errno = EINVAL;
		return NULL;
	}

	entry->timestamp.tm_isdst = -1;

	if(sscanf(p, "%*1[ ][%m[^]]]%*1[ ]", &entry->caller) > 0) {
		p += strlen(entry->caller) + 4;
	} else {
		/* old style entries without caller information */
		p += 1;
	}

	entry->message = strdup(p);

	while((parser->next = fgets(parser->buf, 256, parser->stream)) != NULL) {
		struct tm ts;
		if(strptime(parser->buf, "[%Y-%m-%d %H:%M]", &ts) == NULL) {
			size_t oldlen = strlen(entry->message);
			size_t newlen = oldlen + strlen(parser->buf) + 1;
			char *newmessage = realloc(entry->message, newlen);
			if(oldlen > newlen || newmessage == NULL) {
				errno = ENOMEM;
				return NULL;
			}
			entry->message = newmessage;
			strcpy(entry->message + oldlen, parser->buf);
		} else {
			break;
		}
	}

	return entry;
}

alpm_list_t *pu_log_parse_file(FILE *stream) {
	pu_log_parser_t *parser = pu_log_parser_new(stream);
	pu_log_entry_t *entry;
	alpm_list_t *entries = NULL;
	while((entry = pu_log_parser_next(parser))) {
		entries = alpm_list_add(entries, entry);
	}
	free(parser);
	return entries;
}

pu_log_transaction_status_t pu_log_transaction_parse(const char *message)
{
	const char leader[] = "transaction ";
	const size_t llen = strlen(leader);

	if(strncmp(message, leader, llen) != 0) { return 0; }

	message += llen;
	if(strcmp(message, "started\n") == 0) {
		return PU_LOG_TRANSACTION_STARTED;
	} else if(strcmp(message, "completed\n") == 0) {
		return PU_LOG_TRANSACTION_COMPLETED;
	} else if(strcmp(message, "interrupted\n") == 0) {
		return PU_LOG_TRANSACTION_INTERRUPTED;
	} else if(strcmp(message, "failed\n") == 0) {
		return PU_LOG_TRANSACTION_FAILED;
	}

	return 0;
}

void pu_log_entry_free(pu_log_entry_t *entry)
{
	if(!entry) return;
	free(entry->caller);
	free(entry->message);
	free(entry);
}

/* vim: set ts=2 sw=2 noet: */
