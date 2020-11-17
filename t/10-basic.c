#include <string.h>

#include "tap.c/tap.c"

#include "mGarbageCollector.c"

void freefn(void *buf) {
    strcpy((char *)buf, "bar");
}

void failfn(void *buf) {
    strcpy((char *)buf, "baz");
}

int main(void) {
    char buf[5] = "foo";
    mgc_t mgc = {0};

    tap_plan(4);

    tap_ok(mgc_add(&mgc, buf, freefn) != NULL, "mgc_add");
    mgc_clear(&mgc);
    tap_is_str(buf, "bar", "free function called");

    mgc.failfn = failfn;
    mgc.failctx = buf;
    tap_ok(mgc_add(&mgc, NULL, NULL) == NULL, "mgc_add NULL");
    tap_is_str(buf, "baz", "fail function called");
    mgc_clear(&mgc);

    return 0;
}
