/*
 * Copyright 2017-2020 Andrew Gregory <andrew.gregory.8@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef GLOBDIR_C
#define GLOBDIR_C

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "globdir.h"

static char *_globat_strchrnul(const char *haystack, int c) {
    while(*haystack && *haystack != c) { haystack++; }
    return (char *)haystack;
}

static void _globdir_freepattern(char **parts) {
    char **p;
    if(parts == NULL) { return; }
    for(p = parts; *p != NULL; p++) { free(*p); }
    free(parts);
}

static char **_globdir_split_pattern(const char *pattern) {
    size_t i, count = 1;
    char **parts = NULL;
    const char *c;

    if(pattern == NULL || pattern[0] == '\0') {
        return calloc(sizeof(char*), 1);
    }

    for(c = pattern; *c != '\0'; c++) {
        if(*c == '/') {
            count++;
            while(*c == '/') { c++; }
        }
    }

    if((parts = calloc(sizeof(char*), count + 1)) == NULL) {
        return NULL;
    }

    c = pattern;
    i = 0;
    if(*c == '/') {
        if((parts[i++] = strdup("/")) == NULL) { goto error; }
        while(*c == '/') { c++; }
    }
    while(1) {
        const char *sep = _globat_strchrnul(c, '/');
        if((parts[i++] = strndup(c, sep - c)) == NULL) { goto error; }

        if(sep[0] == '\0') { break; }

        while(sep[1] == '/') { sep++; }

        if(sep[1] == '\0') {
            if((parts[i++] = strdup("/")) == NULL) { goto error; }
            break;
        }
        else {
            c = sep + 1;
        }
    }

    return parts;

error:
    _globdir_freepattern(parts);
    return NULL;
}

static int _globdir_append(globdir_t *pglob, char *path, int flags) {
    char **newmem;
    size_t newsize = pglob->gl_pathc + 2;

    if(flags & GLOB_DOOFFS) { newsize += pglob->gl_offs; }

    if(newsize < pglob->gl_pathc) { errno = ENOMEM; return GLOB_NOSPACE; }
    if(pglob->gl_pathv) {
        newmem = realloc(pglob->gl_pathv, newsize * sizeof(char*));
    } else {
        newmem = calloc(newsize, sizeof(char*));
    }
    if(newmem ==  NULL) { return GLOB_NOSPACE; }

    pglob->gl_pathv = newmem;
    pglob->gl_pathv[pglob->gl_offs + pglob->gl_pathc] = path;
    pglob->gl_pathv[pglob->gl_offs + pglob->gl_pathc + 1] = NULL;
    pglob->gl_pathc++;

    return 0;
}

static int _globcmp(const void *p1, const void *p2) {
    return strcmp(* (char * const *) p1, * (char * const *) p2);
}

static int _globat(int fd, char **pattern, int flags,
        int (*errfunc) (const char *epath, int eerrno),
        globdir_t *pglob, const char *base) {
    const char *epath = (base && base[0]) ? base : ".";
    char path[PATH_MAX];
    DIR *dir;
    struct dirent *entry;
    int fnflags = FNM_PERIOD;

    if(!(flags & GLOB_APPEND)) {
        pglob->gl_pathc = 0;
        pglob->gl_pathv = NULL;
    }
    if(!(flags & GLOB_DOOFFS)) {
        pglob->gl_offs = 0;
    }

    if(flags & GLOB_NOESCAPE) { fnflags |= FNM_NOESCAPE; }
#ifdef GLOB_PERIOD
    if(flags & GLOB_PERIOD) { fnflags &= ~FNM_PERIOD; }
#endif

#define MAYBE_ABORT(p, n) if( (errfunc && errfunc(p, n) != 0) \
        || flags & GLOB_ERR) \
    { return GLOB_ABORTED; }

    if((dir = fdopendir(fd)) == NULL) {
        int err = errno;
        close(fd);
        MAYBE_ABORT(epath, err)
        return 0;
    }

    while((errno = 0, entry = readdir(dir))) {
        struct stat sbuf;

        if(fnmatch(pattern[0], entry->d_name, fnflags) != 0) { continue; }

        if(base) {
            snprintf(path, PATH_MAX, "%s/%s", base, entry->d_name);
        } else {
            snprintf(path, PATH_MAX, "%s", entry->d_name);
        }

        if(fstatat(fd, entry->d_name, &sbuf, 0) != 0) {
            MAYBE_ABORT(path, errno);
            continue;
        }

        if(pattern[1] == NULL) {
            /* pattern is exhausted: match */
            if(S_ISDIR(sbuf.st_mode) && flags & GLOB_MARK) { strcat(path, "/"); }
            if(_globdir_append(pglob, strdup(path), flags) != 0) {
                closedir(dir);
                return GLOB_NOSPACE;
            }
        } else if(!S_ISDIR(sbuf.st_mode)) {
            /* pattern is not exhausted, but entry is a file: no match */
        } else if(pattern[1][0] == '/') {
            /* pattern requires a directory and is exhausted: match */
            strcat(path, "/");
            if(_globdir_append(pglob, strdup(path), flags) != 0) {
                closedir(dir);
                return GLOB_NOSPACE;
            }
        } else {
            /* pattern is not yet exhausted: check directory contents */
            int child = openat(fd, entry->d_name, O_DIRECTORY);
            if(child == -1) {
                MAYBE_ABORT(path, errno);
                continue;
            }

            int ret = _globat(child, pattern + 1,
                    flags | GLOB_APPEND | GLOB_NOSORT, errfunc, pglob, path);
            close(child);
            if(ret != 0) { closedir(dir); return ret; }
        }
    }
    closedir(dir);

    if(errno != 0 && GLOB_ERR) {
        return GLOB_ABORTED;
    }

    if(!(flags & GLOB_NOSORT)) {
        char **p = pglob->gl_pathv;
        if(flags & GLOB_DOOFFS) { p += pglob->gl_offs; }
        qsort(p, pglob->gl_pathc, sizeof(char*), _globcmp);
    }

    return 0;
}

void globdirfree(globdir_t *g) {
	size_t i;
	for (i=0; i < g->gl_pathc; i++)
		free(g->gl_pathv[g->gl_offs + i]);
	free(g->gl_pathv);
	g->gl_pathc = 0;
	g->gl_pathv = NULL;
}

int globat(int fd, const char *pattern, int flags,
        int (*errfunc) (const char *epath, int eerrno), globdir_t *pglob) {
    char **parts, *base;
    int ret;

    if(pattern[0] == '/') {
        base = "";
        fd = open("/", O_DIRECTORY);
        while(pattern[0] == '/') { pattern++; }
    } else {
        fd = openat(fd, ".", O_DIRECTORY);
        base = NULL;
    }

    if(fd == -1) {
        return (flags & GLOB_ERR) ?  GLOB_ABORTED : GLOB_NOMATCH;
    }
    if((parts = _globdir_split_pattern(pattern)) == NULL) {
        close(fd);
        return GLOB_NOSPACE;
    }

    ret = _globat(fd, parts, flags, errfunc, pglob, base);
    _globdir_freepattern(parts);

    if(ret != 0 || pglob->gl_pathc > 0) {
        return ret;
    } else if(flags & GLOB_NOCHECK) {
        return _globdir_append(pglob, strdup(pattern), flags);
    } else {
        return GLOB_NOMATCH;
    }
}

int globdir(const char *dir, const char *pattern, int flags,
        int (*errfunc) (const char *epath, int eerrno), globdir_t *pglob) {
    int fd = open(dir, O_DIRECTORY);
    int ret = globat(fd, pattern, flags, errfunc, pglob);
    close(fd);
    return ret;
}

int globdir_glob(const char *pattern, int flags,
        int (*errfunc) (const char *epath, int eerrno), globdir_t *pglob) {
    return globat(AT_FDCWD, pattern, flags, errfunc, pglob);
}

int globdir_str_is_pattern(const char *string, int noescape) {
    int escape = !noescape;
    for(const char *c = string; *c; c++) {
        switch(*c) {
            case '\\':
                if( escape && *(c + 1) ) { c++; }
                break;
            case '*':
            case '?':
            case '[':
                return 1;
        }
    }
    return 0;
}

char *globdir_escape_pattern(const char *pattern) {
    size_t len, pattern_chars;
    const char *c;
    char *escaped, *e;

    if(pattern == NULL) {
        return NULL;
    }

    len = strlen(pattern);
    pattern_chars = 0;
    for(c = pattern; *c; c++) {
        switch(*c) {
            case '\\':
            case '*':
            case '?':
            case '[':
                pattern_chars++;
        }
    }

    if(pattern_chars == 0) {
        return strdup(pattern);
    }

    if(SIZE_MAX - len < pattern_chars || !(escaped = malloc(len + pattern_chars))) {
        errno = ENOMEM;
        return NULL;
    }

    for(c = pattern, e = escaped; *c; c++, e++) {
        switch(*c) {
            case '\\':
            case '*':
            case '?':
            case '[':
                *e = '\\';
                e++;
                /* fall through */
            default:
                *e = *c;
        }
    }
    *e = '\0';

    return escaped;
}

#endif /* GLOBDIR_C */
