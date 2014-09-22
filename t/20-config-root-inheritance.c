#include "pacutils.h"

#include "tap.h"

char *mockfile;

FILE *fopen(const char *path, const char *mode) {
    if(strcmp(path, "mockfile.ini") == 0) {
        return fmemopen(mockfile, strlen(mockfile), mode);
    } else {
        tap_diag("attempted to open non-mocked file '%s'\n", path);
        return NULL;
    }
}

/* 5 tests */
#define CHECK(r, d, l, g, config_text) do { \
    pu_config_t *config;  \
    mockfile = config_text; \
    config = pu_config_new_from_file("mockfile.ini"); \
    if(!tap_ok(config != NULL, "config != NULL")) { \
        tap_bail("pu_config_new_from_file failed"); \
        return 99; \
    } \
    tap_is_str(config->rootdir, r, "RootDir"); \
    tap_is_str(config->dbpath, d, "DBPath"); \
    tap_is_str(config->logfile, l, "LogFile"); \
    tap_is_str(config->gpgdir, g, "GPGDir"); \
    pu_config_free(config); \
} while(0)

int main(void) {
    /* if RootDir is set and DBPath or LogFile are not
     * they should be relative to RootDir, GPGDir should not */
    tap_plan(5 * 4);
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
