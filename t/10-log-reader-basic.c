#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "pacutils/log.h"

#include "pacutils_test.h"

FILE *stream = NULL;
pu_log_reader_t *reader = NULL;

void cleanup(void) {
	pu_log_reader_free(reader);
	fclose(stream);
}

char buf[] =
"[2016-10-23 09:00] old-style message with no caller\n"
"[2016-10-23 09:00] old-style multi-line message\n"
"continued on line 2...\n"
"and line3\n"
"[2016-10-23 09:00] [mycaller] new-style message with caller\n"
"[2016-10-23 09:00] [mycaller] new-style multi-line message\n"
"continued on line 2...\n"
"and line3\n"
"";

int main(void) {
	pu_log_entry_t *e;

	ASSERT(atexit(cleanup) == 0);
	ASSERT(stream = fmemopen(buf, strlen(buf), "r"));
	ASSERT(reader = pu_log_reader_open_stream(stream));

	tap_plan(18);

	tap_ok((e = pu_log_reader_next(reader)) != NULL, "next");
	tap_is_str(e->caller, NULL, "caller");
	tap_is_str(e->message, "old-style message with no caller\n", "message");
	tap_is_int(reader->eof, 0, "eof");
	pu_log_entry_free(e);

	tap_ok((e = pu_log_reader_next(reader)) != NULL, "next");
	tap_is_str(e->caller, NULL, "caller");
	tap_is_str(e->message, "old-style multi-line message\ncontinued on line 2...\nand line3\n", "message");
	tap_is_int(reader->eof, 0, "eof");
	pu_log_entry_free(e);

	tap_ok((e = pu_log_reader_next(reader)) != NULL, "next");
	tap_is_str(e->caller, "mycaller", "caller");
	tap_is_str(e->message, "new-style message with caller\n", "message");
	tap_is_int(reader->eof, 0, "eof");
	pu_log_entry_free(e);

	tap_ok((e = pu_log_reader_next(reader)) != NULL, "next");
	tap_is_str(e->caller, "mycaller", "caller");
	tap_is_str(e->message, "new-style multi-line message\ncontinued on line 2...\nand line3\n", "message");
	tap_is_int(reader->eof, 0, "eof");
	pu_log_entry_free(e);

	tap_ok(pu_log_reader_next(reader) == NULL, "next");
	tap_ok(reader->eof, "eof");

	return tap_finish();
}
