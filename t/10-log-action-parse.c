#include <errno.h>

#include "pacutils/log.h"

#include "tap.h"

int main(void) {
	pu_log_action_t *a;

	tap_plan(31);

	tap_ok((a = pu_log_action_parse("installed pacutils (1.0.0)")) != NULL, "install");
	tap_is_int(a->operation, PU_LOG_OPERATION_INSTALL, "operation");
	tap_is_str(a->target, "pacutils", "package");
	tap_is_str(a->new_version, "1.0.0", "new_version");
	tap_is_str(a->old_version, NULL, "old_version");
	pu_log_action_free(a);

	tap_ok((a = pu_log_action_parse("upgraded pacutils (1.0.0 -> 2.0.0)")) != NULL, "upgrade");
	tap_is_int(a->operation, PU_LOG_OPERATION_UPGRADE, "operation");
	tap_is_str(a->target, "pacutils", "package");
	tap_is_str(a->new_version, "2.0.0", "new_version");
	tap_is_str(a->old_version, "1.0.0", "old_version");
	pu_log_action_free(a);

	tap_ok((a = pu_log_action_parse("downgraded pacutils (2.0.0 -> 1.0.0)")) != NULL, "downgrade");
	tap_is_int(a->operation, PU_LOG_OPERATION_DOWNGRADE, "operation");
	tap_is_str(a->target, "pacutils", "package");
	tap_is_str(a->new_version, "1.0.0", "new_version");
	tap_is_str(a->old_version, "2.0.0", "old_version");
	pu_log_action_free(a);

	tap_ok((a = pu_log_action_parse("reinstalled pacutils (1.0.0)")) != NULL, "reinstall");
	tap_is_int(a->operation, PU_LOG_OPERATION_REINSTALL, "operation");
	tap_is_str(a->target, "pacutils", "package");
	tap_is_str(a->new_version, "1.0.0", "new_version");
	tap_is_str(a->old_version, "1.0.0", "old_version");
	pu_log_action_free(a);

	tap_ok((a = pu_log_action_parse("removed pacutils (1.0.0)")) != NULL, "removed");
	tap_is_int(a->operation, PU_LOG_OPERATION_REMOVE, "operation");
	tap_is_str(a->target, "pacutils", "package");
	tap_is_str(a->new_version, NULL, "new_version");
	tap_is_str(a->old_version, "1.0.0", "old_version");
	pu_log_action_free(a);

	tap_ok(pu_log_action_parse("") == NULL, "empty input");
	tap_is_int(errno, EINVAL, "errno");

	tap_ok(pu_log_action_parse(NULL) == NULL, "NULL input");
	tap_is_int(errno, EINVAL, "errno");

	tap_ok(pu_log_action_parse("not a valid action") == NULL, "invalid input");
	tap_is_int(errno, EINVAL, "errno");

	return tap_finish();
}
