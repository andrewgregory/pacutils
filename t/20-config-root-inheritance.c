#include <errno.h>

#include "pacutils.h"

#include "tap.h"

char *mockfile;

FILE *fopen(const char *path, const char *mode) {
    if(strcmp(path, "mockfile.ini") == 0) {
        return fmemopen(mockfile, strlen(mockfile), mode);
    } else {
        tap_diag("attempted to open non-mocked file '%s'", path);
        return NULL;
    }
}

/* 5 tests */
#define CHECK(r, d, l, g, config_text) do { \
    pu_config_t *config = pu_config_new(); \
    pu_config_reader_t *reader; \
    mockfile = config_text; \
    reader = pu_config_reader_new(config, "mockfile.ini"); \
    if(config == NULL || reader == NULL) { \
        tap_bail("error initializing reader (%s)", strerror(errno)); \
        return 1; \
    } \
    while(pu_config_reader_next(reader) != -1); \
    tap_ok(reader->eof, "eof reached"); \
    tap_ok(!reader->error, "no error"); \
    tap_ok(pu_config_resolve(config) == 0, "finalize config"); \
    tap_is_str(config->rootdir, r, "RootDir"); \
    tap_is_str(config->dbpath, d, "DBPath"); \
    tap_is_str(config->logfile, l, "LogFile"); \
    tap_is_str(config->gpgdir, g, "GPGDir"); \
    pu_config_reader_free(reader); \
    pu_config_free(config); \
} while(0)

int main(void) {
    /* if RootDir is set and DBPath or LogFile are not
     * they should be relative to RootDir, GPGDir should not */
    tap_plan(7 * 4);
    CHECK("/root", "/dbpath/", "/logfile", "/gpgdir",
            "[options]\n"
            "RootDir = /root\n"
            "DBPath = /dbpath/\n"
            "LogFile = /logfile\n"
            "GPGDir = /gpgdir\n"
        );
    CHECK("/root", "/root/var/lib/pacman/",
            "/root/var/log/pacman.log", "/etc/pacman.d/gnupg/",
            "[options]\n"
            "RootDir = /root\n"
            "#DBPath = /dbpath/\n"
            "#LogFile = /logfile\n"
            "#GPGDir = /gpgdir\n"
        );
    CHECK("/", "/dbpath/", "/logfile", "/gpgdir",
            "[options]\n"
            "#RootDir = /root\n"
            "DBPath = /dbpath/\n"
            "LogFile = /logfile\n"
            "GPGDir = /gpgdir\n"
        );
    CHECK("/", "/var/lib/pacman/", "/var/log/pacman.log", "/etc/pacman.d/gnupg/",
            "[options]\n"
            "#RootDir = /root\n"
            "#DBPath = /dbpath/\n"
            "#LogFile = /logfile\n"
            "#GPGDir = /gpgdir\n"
        );
    return 0;
}
