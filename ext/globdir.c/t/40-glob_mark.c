#include "globdir_test.h"

int cwd = -1;
char *tmpdir = NULL, template[] = "/tmp/40-glob_mark.t-XXXXXX";

void cleanup(void) {
    if(cwd != -1) { fchdir(cwd); close(cwd); }
    if(tmpdir) { rmrfat(AT_FDCWD, tmpdir); }
}

int main(void) {
    globdir_t globbuf;

    ASSERT(atexit(cleanup) == 0);
    ASSERT((cwd = open(".", O_DIRECTORY)) != -1);
    ASSERT(tmpdir = mkdtemp(template));
    ASSERT(chdir(tmpdir) == 0);
    ASSERT(mkdir("foo", 0755) == 0);
    ASSERT(mkdir("foo/bar", 0755) == 0);
    ASSERT(touch("foo/baz", 0755) == 0);

    tap_plan(8);

    tap_is_int(globdir("foo", "ba*", 0, NULL, &globbuf), 0, "globdir return");
    tap_is_int(globbuf.gl_pathc, 2, "glob count");
    tap_is_str(globbuf.gl_pathv[0], "bar", "glob path 1");
    tap_is_str(globbuf.gl_pathv[1], "baz", "glob path 2");
    globfree(&globbuf);

    tap_is_int(globdir("foo", "ba*", GLOB_MARK, NULL, &globbuf), 0, "globdir return");
    tap_is_int(globbuf.gl_pathc, 2, "glob count");
    tap_is_str(globbuf.gl_pathv[0], "bar/", "glob path 1");
    tap_is_str(globbuf.gl_pathv[1], "baz", "glob path 2");
    globfree(&globbuf);

    return tap_finish();
}
