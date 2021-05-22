#include <alpm.h>

#include "pacutils.h"

#include "pacutils_test.h"

int main(void) {
  alpm_file_t files[] = {{.name = "foo/bar"}, {.name = "foo/baz/"}};
  alpm_filelist_t filelist;
  filelist.files = files;
  filelist.count = sizeof(files) / sizeof(*files);

  tap_plan(4);
#define CHECK(in, exp) do { \
    alpm_file_t *f = pu_filelist_contains_path(&filelist, in); \
    tap_is_str(f ? f->name : NULL, exp, in); \
  } while(0);
  CHECK("foo/bar", "foo/bar");
  CHECK("foo/bar/", "foo/bar");
  CHECK("foo//baz/", "foo/baz/");
  CHECK("foo/baz", "foo/baz/");
#undef CHECK
  return tap_finish();
}
