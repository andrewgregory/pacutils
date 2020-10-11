#include "globdir_test.h"

int main(void) {
    tap_skip_all("need to find a way to force readdir to return entries in unsorted order");
    return tap_finish();
}
