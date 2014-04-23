#include "pacutils.c"

#include "tap.h"

int main(int argc, char **argv) {
	int i;
	struct test {
		const char *input;
		const char *expected;
	} tests[] = {
		{ "no whitespace", "no whitespace" },
		{ "   leading whitespace", "leading whitespace" },
		{ "trailing whitespace  ", "trailing whitespace" },
		{ "    both whitespace  ", "both whitespace" },
	};

	tap_plan(sizeof(tests) / sizeof(struct test) * 2);
	for(i = 0; i < sizeof(tests) / sizeof(struct test); ++i) {
		char *output = strdup(tests[i].input);
		int newlen = pu_strtrim(output);
		tap_is_str(output, tests[i].expected, tests[i].input);
		tap_is_int(newlen, strlen(tests[i].expected), NULL);
		free(output);
	}
	return tap_tests_failed();
}

/* vim: set ts=2 sw=2 noet: */
