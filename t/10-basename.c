#include "pacutils/util.h"

#include "pacutils_test.h"

int main(void) {
  tap_plan(5);
#define CHECK(in, exp) tap_is_str(pu_basename(in), exp, #in);
  CHECK("foo/bar", "bar");
  CHECK("foo/bar/", "");
  CHECK("/foo/bar", "bar");
  CHECK("/foo/bar/", "");
  CHECK(NULL, NULL);
#undef CHECK
  return tap_finish();
}
