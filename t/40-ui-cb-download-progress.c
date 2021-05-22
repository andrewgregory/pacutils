#include "pacutils_test.h"
#include "pacutils/ui.h"

pu_ui_ctx_download_t ctx = {
  .update_interval_same = 0,
  .update_interval_next = 0,
};

#define IS(file, event, data, expected, ...) { \
    char *out = NULL; \
    size_t outlen = 0; \
    ctx.out = open_memstream(&out, &outlen); \
    pu_ui_cb_download(&ctx, file, event, data); \
    fclose(ctx.out); \
    _tap_is_str(__FILE__, __LINE__, out, expected, __VA_ARGS__); \
    free(out); \
  }

#define LINE(str) "\x1B[K" str

int main(void) {
  tap_plan(11);

  IS("qux",
      ALPM_DOWNLOAD_INIT,
  &(alpm_download_event_init_t) {0},
  LINE("(1/1) qux (0)\r"),
  "initialize first download");

  IS("bar",
      ALPM_DOWNLOAD_INIT,
  &(alpm_download_event_init_t) {0},
  LINE("(2/2) bar (0)\r"),
  "initialize second download");

  IS("qux",
      ALPM_DOWNLOAD_PROGRESS,
  (&(alpm_download_event_progress_t) { .downloaded = 5, .total = 10 }),
  LINE("(1/2) qux (5/10) 50%\r"),
  "first download progress");

  IS("foo",
      ALPM_DOWNLOAD_INIT,
  (&(alpm_download_event_init_t) {0}),
  LINE("(2/3) bar (0)\r"),
  "initialize third download");

  IS("foo",
      ALPM_DOWNLOAD_PROGRESS,
  (&(alpm_download_event_progress_t) { .downloaded = 3 }),
  LINE("(3/3) foo (3)\r"),
  "third download progress");

  IS("qux",
      ALPM_DOWNLOAD_COMPLETED,
  (&(alpm_download_event_completed_t) { .total = 8 }),
  LINE("qux (8/8) 100%\n") LINE("(1/2) bar (0)\r"),
  "initialize first download");

  IS("bar",
      ALPM_DOWNLOAD_PROGRESS,
  (&(alpm_download_event_progress_t) { .downloaded = 3 }),
  LINE("(2/2) foo (3)\r"),
  "second download progress");

  IS("bar",
      ALPM_DOWNLOAD_RETRY,
  (&(alpm_download_event_retry_t) { .resume = 1 }),
  LINE("(1/2) bar (3)\r"),
  "second download retry with resume");

  IS("foo",
      ALPM_DOWNLOAD_COMPLETED,
  (&(alpm_download_event_completed_t) { .total = 3, .result = 1 }),
  LINE("foo is up to date\n") LINE("(1/1) bar (3)\r"),
  "third download complete (up to date)");

  IS("bar",
      ALPM_DOWNLOAD_RETRY,
  (&(alpm_download_event_retry_t) { .resume = 0 }),
  LINE("(1/1) bar (0)\r"),
  "second download retry without resume");

  IS("bar",
      ALPM_DOWNLOAD_COMPLETED,
  (&(alpm_download_event_completed_t) { .result = -1 }),
  LINE("bar failed to download\n"),
  "second download complete (failed to download)");

  return tap_finish();
}
