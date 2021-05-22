#include <stdarg.h>
#include <stdlib.h>

#include "../ext/tap.c/tap.c"

#include "../globdir.c"

#define IS(p, e) { \
    char *escaped = globdir_escape_pattern(p); \
    tap_is_str(escaped, e, "%s -> %s", p ? p : "NULL", e ? e : "NULL"); \
    free(escaped); \
}

int main(void) {
    tap_plan(4);

    IS( "foo/bar", "foo/bar" );
    IS( "fo*/bar", "fo\\*/bar" );
    IS( "\\?*[", "\\\\\\?\\*\\[" );
    IS( NULL, NULL );

    return tap_finish();
}
