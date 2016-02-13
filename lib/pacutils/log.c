#define _XOPEN_SOURCE 700 /* strptime/strndup */

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

static const char *_pu_strrstr(const char *haystack, const char *start, const char *needle)
{
	ssize_t nlen = strlen(needle);
	if(start - haystack < nlen) { return NULL; }
	for(start -= nlen; start > haystack; start--) {
		if(memcmp(start, needle, nlen) == 0) { return start; }
	}
	return NULL;
}

pu_log_action_t *pu_log_action_parse(char *message)
{
	pu_log_action_t *a;
	size_t mlen = strlen(message);

	if(mlen <= 10) { errno = EINVAL; return NULL; }
	if(message[mlen - 1] == '\n') { mlen--; }
	if(message[mlen - 1] != ')') { errno = EINVAL; return NULL; }

	if((a = calloc(sizeof(pu_log_action_t), 1)) == NULL) { return NULL; }

#define PU_STARTSWITH(n) (mlen >= sizeof(n) && memcmp(message, n, sizeof(n) - 1) == 0)
	if(PU_STARTSWITH("upgraded ")) {
		const char *pkg = message + sizeof("upgraded ") - 1;
		const char *sep = _pu_strrstr(message, message + mlen, " -> ");
		const char *op = _pu_strrstr(message, sep, " (");
		if(pkg[0] == '\0' || sep == NULL || op == NULL) { errno = EINVAL; goto error; }
		a->operation = PU_LOG_OPERATION_UPGRADE;
		if((a->target = strndup(pkg, op - pkg)) == NULL) { goto error; }
		if((a->old_version = strndup(op + 2, sep - op - 2)) == NULL) { goto error; }
		if((a->new_version = strndup(sep + 4, mlen - (sep + 4 - message))) == NULL) { goto error; }
	} else if(PU_STARTSWITH("downgraded ")) {
		const char *pkg = message + sizeof("downgraded ") - 1;
		const char *sep = _pu_strrstr(message, message + mlen, " -> ");
		const char *op = _pu_strrstr(message, sep, " (");
		if(pkg[0] == '\0' || sep == NULL || op == NULL) { errno = EINVAL; goto error; }
		a->operation = PU_LOG_OPERATION_DOWNGRADE;
		if((a->target = strndup(pkg, op - pkg)) == NULL) { goto error; }
		if((a->old_version = strndup(op + 2, sep - op - 2)) == NULL) { goto error; }
		if((a->new_version = strndup(sep + 4, mlen - (sep + 4 - message))) == NULL) { goto error; }
	} else if(PU_STARTSWITH("installed ")) {
		const char *pkg = message + sizeof("installed ") - 1;
		const char *op = _pu_strrstr(message, message + mlen, " (");
		if(pkg[0] == '\0' || op == NULL) { errno = EINVAL; goto error; }
		a->operation = PU_LOG_OPERATION_INSTALL;
		if((a->target = strndup(pkg, op - pkg)) == NULL) { goto error; }
		if((a->new_version = strndup(op + 2, mlen - (op + 2 - message))) == NULL) { goto error; }
	} else if(PU_STARTSWITH("reinstalled ")) {
		const char *pkg = message + sizeof("reinstalled ") - 1;
		const char *op = _pu_strrstr(message, message + mlen, " (");
		if(pkg[0] == '\0' || op == NULL) { errno = EINVAL; goto error; }
		a->operation = PU_LOG_OPERATION_REINSTALL;
		if((a->target = strndup(pkg, op - pkg)) == NULL) { goto error; }
		if((a->old_version = strndup(op + 2, mlen - (op + 2 - message))) == NULL) { goto error; }
		if((a->new_version = strdup(a->old_version)) == NULL) { goto error; }
	} else if(PU_STARTSWITH("uninstalled ")) {
		const char *pkg = message + sizeof("uninstalled ") - 1;
		const char *op = _pu_strrstr(message, message + mlen, " (");
		if(pkg[0] == '\0' || op == NULL) { errno = EINVAL; goto error; }
		a->operation = PU_LOG_OPERATION_REMOVE;
		if((a->target = strndup(pkg, op - pkg)) == NULL) { goto error; }
		if((a->old_version = strndup(op + 2, mlen - (op + 2 - message))) == NULL) { goto error; }
	} else {
		errno = EINVAL;
		goto error;
	}
#undef PU_STARTSWITH

	return a;

error:
	pu_log_action_free(a);
	return NULL;
}

int pu_log_fprint_entry(FILE *stream, pu_log_entry_t *entry)
{
	char timestamp[50];

	strftime(timestamp, 50, "%F %R", &entry->timestamp);
	if(entry->caller) {
		return fprintf(stream, "[%s] [%s] %s",
				timestamp, entry->caller, entry->message);
	} else {
		return fprintf(stream, "[%s] %s", timestamp, entry->message);
	}
}

pu_log_parser_t *pu_log_parser_new(FILE *stream) {
	pu_log_parser_t *parser = calloc(sizeof(pu_log_parser_t), 1);
	parser->stream = stream;
	return parser;
}

void pu_log_parser_free(pu_log_parser_t *p) {
	if(p) { free(p); }
}

pu_log_entry_t *pu_log_parser_next(pu_log_parser_t *parser) {
	char *p, *c;
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

	if(p[0] == ' ' && p[1] == '[' && (c = strstr(p + 2, "] "))) {
		entry->caller = strndup(p + 2, c - (p + 2));
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
