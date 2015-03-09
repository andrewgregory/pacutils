#include <alpm.h>

#include "tap.h"

#include "pacutils.h"

int main(void) {
    alpm_list_t *l = NULL, *n, *s;
    char *data1 = "foo", *data2 = "bar";

    tap_plan(13);

    n = _pu_list_append(&l, data1);
    tap_ok(l != NULL, NULL);
    tap_ok(l == n, NULL);
    tap_ok(l->next == NULL, NULL);
    tap_ok(l->prev == l, NULL);
    tap_ok(l->data == data1, NULL);

    s = l;
    n = _pu_list_append(&l, data2);
    tap_ok(l == s, NULL);
    tap_ok(l->prev == n, NULL);
    tap_ok(l->next == n, NULL);
    tap_ok(l->data == data1, NULL);

    tap_ok(n != NULL, NULL);
    tap_ok(n->next == NULL, NULL);
    tap_ok(n->prev == l, NULL);
    tap_ok(n->data == data2, NULL);

    return tap_finish();
}
