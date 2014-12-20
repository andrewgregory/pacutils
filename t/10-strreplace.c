#include "pacutils.c"

#include "tap.h"

int main(void) {
    tap_plan(8);
#define CHECK(in, tgt, repl, exp, desc) do { \
    char *got = pu_strreplace(in, tgt, repl); \
    tap_is_str(got, exp, desc); \
    free(got); \
} while(0);
    CHECK("foobarbaz", "bar", "qux", "fooquxbaz", "equal length");
    CHECK("foobarbaz", "bar", "quux", "fooquuxbaz", "longer replacement");
    CHECK("foobarbaz", "bar", "qx", "fooqxbaz", "shorter replacement");
    CHECK("foobarbaz", "foo", "qux", "quxbarbaz", "startswith target");
    CHECK("foobarbaz", "baz", "qux", "foobarqux", "endswith target");
    CHECK("foobarbaz", "a", "x", "foobxrbxz", "multiple instances");
    CHECK("barbarbar", "barbar", "foo", "foobar", "overlapping instances");
    CHECK("foo", "foo", "bar", "bar", "equals target");
#undef CHECK
    return tap_finish();
}
