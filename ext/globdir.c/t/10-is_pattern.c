#include <stdarg.h>
#include <stdlib.h>

#include "../ext/tap.c/tap.c"

#include "../globdir.c"

int main(void) {
    tap_plan(8);

    tap_ok( globdir_str_is_pattern("foo*", 0), "foo*" );
    tap_ok( globdir_str_is_pattern("foo?", 0), "foo?" );
    tap_ok( globdir_str_is_pattern("fo[o]", 0), "fo[o]" );
    tap_ok( globdir_str_is_pattern("foo\\*", 1), "foo\\* (NOESCAPE)" );

    tap_ok( !globdir_str_is_pattern("foo", 0), "foo" );
    tap_ok( !globdir_str_is_pattern("foo\\*", 0), "foo\\*" );
    tap_ok( !globdir_str_is_pattern("foo\\", 0), "foo\\" );
    tap_ok( !globdir_str_is_pattern("foo\\", 1), "foo\\ (NOESCAPE)" );

    return tap_finish();
}
