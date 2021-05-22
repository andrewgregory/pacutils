#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "tap.c/tap.c"

#include "mini.c"

int main(void)
{
    char buf[] =
        "\n"
        " key outside section\n"
        " [ section with spaces ] \n"
        " [key \n"
        " key = value \n"
        " \n"
        " # comment-only line \n"
        " key with spaces \n"
        "key=value\n"
        "key#this is a comment\n"
        " [] # empty section name\n"
        "emptysection\n";
    FILE *stream;
    mini_t *ini;

    tap_plan(34);

    if((stream = fmemopen(buf, strlen(buf), "r")) == NULL) {
        tap_bail("error: could not open memory stream (%s)\n", strerror(errno));
        return 1;
    }
    if((ini = mini_finit(stream)) == NULL) {
        tap_bail("error: could not init mini parser (%s)\n", strerror(errno));
        return 1;
    }

#define CHECKLN(ln, s, k, v) {                                                \
        tap_ok(mini_lookup_key(ini, s, k) != NULL, "lookup(%s, %s)", #s, #k); \
        tap_is_int(ini->lineno, ln, "line %d lineno", ln);                    \
        tap_is_str(ini->section, s, "line %d section", ln);                   \
        tap_is_str(ini->key, k, "line %d key", ln);                           \
        tap_is_str(ini->value, v, "line %d value", ln);                       \
        tap_is_int(ini->eof, 0, "line %d eof", ln);                           \
    }

    tap_ok(ini != NULL, "mini_init");

    CHECKLN(12, "", "emptysection", NULL);
    CHECKLN(8, " section with spaces ", "key with spaces", NULL);
    CHECKLN(5, " section with spaces ", "key", "value");
    CHECKLN(4, " section with spaces ", "[key", NULL);
    CHECKLN(2, NULL, "key outside section", NULL);

    tap_ok(mini_lookup_key(ini, "non-existent", "non-existent") == NULL, NULL);
    tap_is_int(ini->lineno, 12, NULL);
    tap_ok(ini->eof, NULL);

#undef CHECKLN

    mini_free(ini);
    fclose(stream);

    return 0;
}
