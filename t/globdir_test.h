#ifndef GLOBDIR_TEST_H
#define GLOBDIR_TEST_H

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "../ext/tap.c/tap.c"

#include "../globdir.c"

#define ASSERT(x) if(!(x)) { tap_bail("ASSERT FAILED (%s)", #x); exit(1); }

int rmrfat(int dd, const char *path) {
    struct stat sbuf;
    if(fstatat(dd, path, &sbuf, AT_SYMLINK_NOFOLLOW) != 0) {
        return errno == ENOENT ? 0 : -1;
    } else if(S_ISDIR(sbuf.st_mode)) {
        struct dirent *ent;
        int fd;
        DIR *d;

        if((fd = openat(dd, path, O_DIRECTORY)) < 0) { return -1; }
        if((d = fdopendir(fd)) == NULL) { close(fd); return -1; }

        while(errno = 0, ent = readdir(d)) {
            if(strcmp(ent->d_name, ".") == 0) { continue; }
            if(strcmp(ent->d_name, "..") == 0) { continue; }
            if(rmrfat(fd, ent->d_name) != 0) { break; }
        }
        closedir(d);
        return errno == 0 ? unlinkat(dd, path, AT_REMOVEDIR) : -1;
    } else {
        return unlinkat(dd, path, 0);
    }
}

int touch(const char *path, mode_t mode) {
    int fd = open(path, O_RDONLY | O_CREAT, mode);
    if(fd == -1) { return -1; }
    close(fd);
    return 0;
}

#endif
