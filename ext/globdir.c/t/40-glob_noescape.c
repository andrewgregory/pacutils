#include "globdir_test.h"

int cwd = -1;
char *tmpdir = NULL, template[] = "/tmp/40-glob_noescape.t-XXXXXX";

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
    ASSERT(touch("foo/b\\ar", 0755) == 0);

    tap_plan(5);

    /* try to match literal "b*ar" */
    tap_is_int(globdir("foo", "b\\*ar", 0, NULL, &globbuf), GLOB_NOMATCH, "globdir return");
    tap_is_int(globbuf.gl_pathc, 0, "globdir count");
    globfree(&globbuf);

    /* try to match literal '\', '*' retains special meaning */
    tap_is_int(globdir("foo", "b\\*ar", GLOB_NOESCAPE, NULL, &globbuf), 0, "globdir return");
    tap_is_int(globbuf.gl_pathc, 1, "globdir count");
    tap_is_str(globbuf.gl_pathv[0], "b\\ar", "glob path 1");
    globfree(&globbuf);

    return tap_finish();
}
