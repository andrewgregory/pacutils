#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "pacutils.h"

#include "tap.h"

char buf[] =
    "[Trigger]\n"
    "Operation = Install\n"
    "Operation = Upgrade\n"
    "Type = Package\n"
    "Type = File\n"
    "Target = foo\n"
    "Target = bar\n"
    "\n"
    "[Trigger]\n"
    "Operation = Install\n"
    "Operation = Upgrade\n"
    "Type = Package\n"
    "Type = File\n"
    "Target = foo\n"
    "Target = bar\n"
    "\n"
    "[Action]\n"
    "Description = test hook\n"
    "AbortOnFail\n"
    "NeedsTargets\n"
    "Exec = foo \"bar baz\"\n"
    "";

int main(void) {
    FILE *stream;
    pu_hook_t *hook;
    pu_hook_reader_t *reader;

    tap_assert(stream = fmemopen(buf, sizeof(buf), "r"));
    tap_assert(hook = pu_hook_new());
    tap_assert(reader = pu_hook_reader_finit(hook, stream));

	tap_plan(5);

    while(pu_hook_reader_next(reader) != -1);

    tap_ok(reader->eof, "eof reached");
    tap_ok(!reader->error, "no error");

    tap_is_str(hook->desc, "test hook", "Description");
    tap_ok(hook->abort_on_fail, "AbortOnFail");
    tap_ok(hook->needs_targets, "NeedsTargets");

    pu_hook_reader_free(reader);
    pu_hook_free(hook);

	return tap_finish();
}
