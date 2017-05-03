#include "pacutils.h"

#include "pacutils_test.h"

#define CHECK(p1, p2, exp) do { \
	int match = pu_pathcmp(p1, p2) ? 1 : 0; \
	tap_ok(match == exp, exp ? p1 " != " p2 : p1 " == " p2); \
} while(0)

int main(void) {
	tap_plan(6);
	CHECK("/foo/bar", "/foo/bar", 0);
	CHECK("//foo/bar", "/foo/bar", 0);
	CHECK("/foo/bar//", "/foo/bar", 0);
	CHECK("/foo//bar", "/foo/bar", 0);
	CHECK("foo/bar", "/foo/bar", 1);
	CHECK("/foo/bar", "/bar/foo", 1);
	return tap_finish();
}
