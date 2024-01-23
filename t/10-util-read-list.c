#include <string.h>
#include <stdlib.h>

#include <alpm_list.h>

#include "pacutils.h"
#include "pacutils_test.h"

#define is_str_list(l, str, desc) do { \
    if(l) { \
      tap_is_str(l->data, str, desc); \
      l = alpm_list_next(l); \
    } else { \
      tap_ok(l != NULL, desc); \
    } \
  } while(0)

#define is_list_exhausted(l, name) do { \
    tap_ok(l == NULL, name " exhausted"); \
    if(l) { \
      tap_diag("remaining elements:"); \
      while(l) { \
        tap_diag("%s", (char*) l->data); \
        l = alpm_list_next(l); \
      } \
    } \
  } while(0)

FILE *stream = NULL;

void cleanup(void) {
  fclose(stream);
}

char buf[] =
  "foo\n"
  "bar\n"
  "baz\n"
  "";

int main(void) {
  alpm_list_t *dest = NULL;

  ASSERT(atexit(cleanup) == 0);
  ASSERT(stream = fmemopen(buf, strlen(buf), "r"));

  tap_plan(5);

  tap_is_int(pu_read_list_from_stream(stream, '\n', &dest), 0, "read successful");
  is_str_list(dest, "foo", "foo");
  is_str_list(dest, "bar", "bar");
  is_str_list(dest, "baz", "baz");
  is_list_exhausted(dest, "dest");

  return tap_finish();
}

/* vim: set ts=2 sw=2 et: */
