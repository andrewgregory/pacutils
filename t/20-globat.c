#include "globdir_test.h"

int cwd = -1;
char *tmpdir = NULL, template[] = "/tmp/20-globat.t-XXXXXX";

void cleanup(void) {
    if(cwd != -1) { fchdir(cwd); close(cwd); }
    if(tmpdir) { rmrfat(AT_FDCWD, tmpdir); }
}

int main(void) {
    int fd;
    globdir_t globbuf;

    ASSERT(atexit(cleanup) == 0);
    ASSERT((cwd = open(".", O_DIRECTORY)) != -1);
    ASSERT(tmpdir = mkdtemp(template));
    ASSERT(chdir(tmpdir) == 0);
    ASSERT(mkdir("foo", 0755) == 0);
    ASSERT(mkdir("foo/bar", 0755) == 0);
    ASSERT(touch("foo/bar/quux", 0755) == 0);
    ASSERT(touch("foo/baz", 0755) == 0);
    ASSERT((fd = open("foo", O_DIRECTORY)) != -1);

    tap_plan(12);

    tap_is_int(globat(fd, "ba*", 0, NULL, &globbuf), 0, "globat return");
    tap_is_int(globbuf.gl_pathc, 2, "glob count");
    tap_is_str(globbuf.gl_pathv[0], "bar", "glob path 1");
    tap_is_str(globbuf.gl_pathv[1], "baz", "glob path 2");
    globdirfree(&globbuf);

    tap_is_int(globat(fd, "ba*/", 0, NULL, &globbuf), 0, "globat return");
    tap_is_int(globbuf.gl_pathc, 1, "glob count");
    tap_is_str(globbuf.gl_pathv[0], "bar/", "glob path 1");
    globdirfree(&globbuf);

    tap_is_int(globat(fd, "bar/quux", 0, NULL, &globbuf), 0, "globat return");
    tap_is_int(globbuf.gl_pathc, 1, "glob count");
    tap_is_str(globbuf.gl_pathv[0], "bar/quux", "glob path 1");
    globdirfree(&globbuf);

    tap_is_int(globat(fd, "quux", 0, NULL, &globbuf), GLOB_NOMATCH, "globat return");
    tap_is_int(globbuf.gl_pathc, 0, "glob count");
    globdirfree(&globbuf);

    return tap_finish();
}
