#include "../tap.c"

/* just verify the library compiles */

int main(void) {
    tap_plan(1);
    tap_ok(1, "compilation successful");
    return 0;
}
