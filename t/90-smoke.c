#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "tap.c/tap.c"

#include "mini.c"

#define SECTIONS 10000

int main(void)
{
    char *buf;
    size_t i, buf_siz;
    mini_t *ini;

    FILE *stream = open_memstream(&buf, &buf_siz);
    if(stream == NULL) {
        tap_bail("error: could not open memory stream (%s)\n", strerror(errno));
        return 1;
    }
    for(i = 1; i <= SECTIONS; i++) {
        fprintf(stream, " [ section %zu ] \n", i);
        fprintf(stream, " # comment \n");
        fprintf(stream, " key%zu = value%zu #inline comment \n", i, i);
        fprintf(stream, " # comment \n");
        fprintf(stream, " key%zu = value%zu #inline comment \n", i, i);
        fprintf(stream, " # comment \n");
        fprintf(stream, " key%zu = value%zu #inline comment \n", i, i);
        fprintf(stream, " # comment \n");
    }
    fprintf(stream, "[target section]\n");
    fprintf(stream, "target key = target value\n");
    fflush(stream);

    if((ini = mini_finit(stream)) == NULL) {
        tap_bail("error: could not init mini parser (%s)\n", strerror(errno));
        return 1;
    }

    tap_plan(3);
    tap_ok(mini_lookup_key(ini, "target section", "target key") != NULL, "lookup");
    tap_is_str(ini->value, "target value", "value");
    tap_is_int(ini->lineno, SECTIONS * 8 + 2, "lineno");

    mini_free(ini);
    fclose(stream);
    free(buf);

    return 0;
}
