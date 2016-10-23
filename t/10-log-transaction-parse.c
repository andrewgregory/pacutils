#include <errno.h>

#include "pacutils/log.h"

#include "tap.h"

int main(void) {
	tap_plan(10);

	tap_is_int(pu_log_transaction_parse("transaction started\n"),
			PU_LOG_TRANSACTION_STARTED, "started");
	tap_is_int(pu_log_transaction_parse("transaction completed\n"),
			PU_LOG_TRANSACTION_COMPLETED, "completed");
	tap_is_int(pu_log_transaction_parse("transaction interrupted\n"),
			PU_LOG_TRANSACTION_INTERRUPTED, "interrupted");
	tap_is_int(pu_log_transaction_parse("transaction failed\n"),
			PU_LOG_TRANSACTION_FAILED, "failed");

	tap_is_int(pu_log_transaction_parse("invalid"), 0, "invalid input");
	tap_is_int(errno, EINVAL, "errno");

	tap_is_int(pu_log_transaction_parse(""), 0, "empty input");
	tap_is_int(errno, EINVAL, "errno");

	tap_is_int(pu_log_transaction_parse(NULL), 0, "NULL input");
	tap_is_int(errno, EINVAL, "errno");

	return tap_finish();
}
