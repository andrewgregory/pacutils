#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "../ext/tap.c/tap.c"

#ifndef PACUTILS_TEST_H
#define PACUTILS_TEST_H

#define ASSERT(x) if(!(x)) { tap_bail("ASSERT FAILED: %s - %s", __FILE__, #x); exit(1); }

int rmrfat(int dd, const char *path) {
    if(!unlinkat(dd, path, 0)) {
        return 0;
    } else {
        struct dirent *de;
        DIR *d;
        int fd;

        switch(errno) {
            case ENOENT:
                return 0;
            case EPERM:
            case EISDIR:
                break;
            default:
                /* not a directory */
                return 0;
        }

        fd = openat(dd, path, O_DIRECTORY);
        d = fdopendir(fd);
        if(!d) { return 0; }
        for(de = readdir(d); de != NULL; de = readdir(d)) {
            if(strcmp(de->d_name, "..") != 0 && strcmp(de->d_name, ".") != 0) {
                char name[PATH_MAX];
                snprintf(name, PATH_MAX, "%s/%s", path, de->d_name);
                rmrfat(dd, name);
            }
        }
        closedir(d);
        unlinkat(dd, path, AT_REMOVEDIR);
    }
    return 0;
}

int vspew(int dd, const char *path, const char *format, va_list vargs) {
	int fd = openat(dd, path, O_WRONLY | O_CREAT, 0777);
	if(fd == -1) { return -1; }
	if(vdprintf(fd, format, vargs) < 0) { close(fd); return -1; }
	close(fd);
	return 0;
}

int spew(int dd, const char *path, const char *format, ...) {
  int ret;
  va_list args;

  va_start(args, format);
  ret = vspew(dd, path, format, args);
  va_end(args);

  return ret;
}

#endif /* PACUTILS_TEST_H */
