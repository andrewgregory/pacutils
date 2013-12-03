#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

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

	strftime(timestamp, 50, "%F %R", entry->timestamp);
	if(entry->caller) {
		return printf("[%s] [%s] %s", timestamp, entry->caller, entry->message);
	} else {
		return printf("[%s] %s", timestamp, entry->message);
	}
}

alpm_list_t *pu_log_parse_file(FILE *stream)
{
	alpm_list_t *entries = NULL;
	char buf[256];

	while(fgets(buf, 256, stream)) {
		char *p, *caller;
		struct tm *timestamp = calloc(sizeof(struct tm), 1);

		if(!(p = strptime(buf, "[%Y-%m-%d %H:%M]", timestamp))) {
			/* line is a continuation of the previous line */
			alpm_list_t *last = alpm_list_last(entries);

			if(!last) {
				return NULL;
			}

			pu_log_entry_t *entry = last->data;
			size_t newlen = strlen(entry->message) + strlen(buf) + 1;
			entry->message = realloc(entry->message, newlen);
			strcat(entry->message, buf);

			free(timestamp);

			continue;
		}

		pu_log_entry_t *entry = calloc(sizeof(pu_log_entry_t), 1);

		entry->timestamp = timestamp;

		if(sscanf(p, "%*1[ ][%m[^]]]%*1[ ]", &caller) > 0) {
			entry->caller = caller;
			p += strlen(caller) + 4;
		} else {
			/* old style entries without caller information */
			p += 1;
		}

		entry->message = strdup(p);

		entries = alpm_list_add(entries, entry);
	}

	return entries;
}

void pu_log_entry_free(pu_log_entry_t *entry)
{
	if(!entry) return;
	free(entry->timestamp);
	free(entry->caller);
	free(entry->message);
	free(entry);
}

/* vim: set ts=2 sw=2 noet: */
