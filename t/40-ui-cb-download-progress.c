#include "capture.c"
#include "pacutils_test.h"
#include "pacutils/ui.h"

typedef struct {
    char *filename;
    off_t xfer, total;
} cb_ctx;
void caller(void *ctx) { cb_ctx *c = ctx; pu_ui_cb_download(c->filename, c->xfer, c->total); }

#define IS(file, xfer, total, expected, ...) { \
    char *out; \
    cb_ctx ctx = { file, xfer, total }; \
    capture(caller, &ctx, &out); \
    _tap_is_str(__FILE__, __LINE__, out, expected, __VA_ARGS__); \
    free(out); \
}

int main(void) {
    tap_plan(8);

    IS("qux", 0, -1, "downloading qux (0)\r", "initialized download");
    IS("qux", 0, 2, "downloading qux (0/2) 0%\r", "started download");
    IS("foo", 1, 2, "downloading foo (1/2) 50%\r", "partial download");
    IS("bar", 1, 0, "downloading bar (1)\r", "partial download, unknown size");
    IS("baz", 1, 1, "downloading baz (1/1) 100%\n", "completed download");

    IS("qux", 0, 0, "", "non-transfer event");
    IS("foo", -1, 1, "", "invalid xfer value");
    IS("foo", 1, -1, "downloading foo (1)\r", "invalid total size");

    return tap_finish();
}
