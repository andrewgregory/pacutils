#include "pacutils.c"

#include "tap.h"

#define CHECK(in, exp) do { \
		char *c = strdup(in); \
		size_t newlen = pu_strtrim(c); \
		tap_is_str(c, exp, "pu_strtrim(%s)", in); \
		tap_is_int((int) newlen, strlen(exp), "strlen(%s)", c); \
		free(c); \
	} while(0)

int main(void) {
	tap_plan(2 * 6);
	CHECK("no whitespace", "no whitespace" );
	CHECK("   leading whitespace", "leading whitespace" );
	CHECK("trailing whitespace  ", "trailing whitespace" );
	CHECK("    both whitespace  ", "both whitespace" );
	CHECK("                     ", "");
	CHECK("", "");
	return 0;
}

/* vim: set ts=2 sw=2 noet: */
