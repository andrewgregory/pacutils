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
} pu_log_parser_t;

pu_log_transaction_status_t pu_log_transaction_parse(const char *message);

int pu_log_fprint_entry(FILE *stream, pu_log_entry_t *entry);
pu_log_entry_t *pu_log_parser_next(pu_log_parser_t *parser);
pu_log_parser_t *pu_log_parser_new(FILE *stream);
alpm_list_t *pu_log_parse_file(FILE *stream);
void pu_log_entry_free(pu_log_entry_t *entry);

pu_log_action_t *pu_log_action_parse(char *message);
void pu_log_action_free(pu_log_action_t *action);

#endif

/* vim: set ts=2 sw=2 noet: */
