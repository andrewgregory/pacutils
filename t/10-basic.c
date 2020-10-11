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

    tap_plan(59);

    if((stream = fmemopen(buf, strlen(buf), "r")) == NULL) {
        tap_bail("error: could not open memory stream (%s)\n", strerror(errno));
        return 1;
    }
    if((ini = mini_finit(stream)) == NULL) {
        tap_bail("error: could not init mini parser (%s)\n", strerror(errno));
        return 1;
    }

#define CHECKLN(ln, s, k, v) {                                         \
        tap_ok(mini_next(ini) != NULL, "line %d next != NULL", ln);    \
        tap_is_int(ini->lineno, ln, "line %d lineno", ln);             \
        tap_is_str(ini->section, s, "line %d section", ln);            \
        tap_is_str(ini->key, k, "line %d key", ln);                    \
        tap_is_str(ini->value, v, "line %d value", ln);                \
        tap_is_int(ini->eof, 0, "line %d eof", ln);                    \
    }

    tap_ok(ini != NULL, "mini_init");

    CHECKLN(2, NULL, "key outside section", NULL);
    CHECKLN(3, " section with spaces ", NULL, NULL);
    CHECKLN(4, " section with spaces ", "[key", NULL);
    CHECKLN(5, " section with spaces ", "key", "value");
    CHECKLN(8, " section with spaces ", "key with spaces", NULL);
    CHECKLN(9, " section with spaces ", "key", "value");
    CHECKLN(10, " section with spaces ", "key", NULL);
    CHECKLN(11, "", NULL, NULL);
    CHECKLN(12, "", "emptysection", NULL);

    tap_ok(mini_next(ini) == NULL, "next(eof) == NULL");
    tap_is_int(ini->lineno, 12, "final line count");
    tap_ok(feof(ini->stream), "feof(ini->stream == 1");
    tap_ok(ini->eof, "ini->eof == 1");

#undef CHECKLN

    mini_free(ini);
    fclose(stream);

    return 0;
}
