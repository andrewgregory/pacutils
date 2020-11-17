/*
 * Copyright 2020 Andrew Gregory <andrew.gregory.8@gmail.com>
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
 *
 * Project URL: http://github.com/andrewgregory/mGarbageCollector.c
 */

#ifndef MGARBAGECOLLECTOR_C
#define MGARBAGECOLLECTOR_C

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mGarbageCollector.h"

void *mgc_add(mgc_t *mgc, void *ptr, mgc_fn_free_t* fn) {
    mgc_item_t *item;

    if(ptr == NULL || fn == NULL) {
        if(mgc->failfn) { mgc->failfn(mgc->failctx); }
        return NULL;
    }

    if((item = malloc(sizeof(mgc_item_t))) == NULL) {
        fn(ptr);
        if(mgc->failfn) { mgc->failfn(mgc->failctx); }
        return NULL;
    }

    item->data = ptr;
    item->free = fn;
    item->next = NULL;

    if(mgc->items) {
        mgc->last->next = item;
    } else {
        mgc->items = item;
    }
    mgc->last = item;

    return item->data;
}

mgc_fn_free_t *mgc_remove_item(mgc_t *mgc, void *ptr) {
    mgc_item_t *i, *prev;
    for(prev = NULL, i = mgc->items; i; prev = i, i = i->next) {
        if(i->data == ptr) {
            mgc_fn_free_t *freefn = i->free;

            if(i == mgc->last) { mgc->last = prev; }
            if(i == mgc->items) { mgc->items = i->next; }
            else { prev->next = i->next; }

            free(i);
            return freefn;
        }
    }
    return NULL;
}

void mgc_free_item(mgc_t *mgc, void *ptr) {
    mgc_fn_free_t *freefn = mgc_remove_item(mgc, ptr);
    if(freefn) { freefn(ptr); }
}

mgc_t *mgc_new(mgc_fn_fail_t *failfn, void *failctx) {
    mgc_t *mgc = calloc(1, sizeof(mgc_t));
    if(mgc) {
        mgc->failfn = failfn;
        mgc->failctx = failctx;
    }
    return mgc;
}

void mgc_clear(mgc_t *mgc) {
    while(mgc->items) {
        mgc_item_t *next = mgc->items->next;
        mgc->items->free(mgc->items->data);
        free(mgc->items);
        mgc->items = next;
    }
    mgc->items = NULL;
}

void mgc_free(mgc_t *mgc) {
    if(mgc) {
        mgc_clear(mgc);
        free(mgc);
    }
}

/*************************************
 * syntactic sugar
 *************************************/

/* stdlib */

void *mgc_malloc(mgc_t *mgc, size_t size) {
    return mgc_add(mgc, malloc(size), free);
}

void *mgc_calloc(mgc_t *mgc, size_t nelem, size_t elsize) {
    return mgc_add(mgc, calloc(nelem, elsize), free);
}

void *mgc_realloc(mgc_t *mgc, void *ptr, size_t size) {
    mgc_fn_free_t *freefn;
    void *newptr = realloc(ptr, size);
    if(newptr && (freefn = mgc_remove_item(mgc, ptr))) {
        mgc_add(mgc, newptr, freefn);
    }
    return newptr;
}

/* string */

char *mgc_strdup(mgc_t *mgc, const char *s) {
    return mgc_add(mgc, strdup(s), free);
}

char *mgc_strndup(mgc_t *mgc, const char *s, size_t size) {
    return mgc_add(mgc, strndup(s, size), free);
}

char *mgc_vasprintf(mgc_t *mgc, const char *restrict fmt, va_list ap) {
    va_list ap_copy;
    char *p;
    int len;

    va_copy(ap_copy, ap);
    len = vsnprintf(NULL, 0, fmt, ap_copy);
    va_end(ap_copy);

    if(len < 0) {
        return mgc_add(mgc, NULL, NULL);
    }

#if SIZE_MAX <= INT_MAX
    if(len >= SIZE_MAX) {
        errno = EOVERFLOW;
        return mgc_add(mgc, NULL, NULL);
    }
#endif

    if((p = mgc_malloc(mgc, (size_t)len + 1)) == NULL) {
        return NULL;
    }

    vsprintf(p, fmt, ap);

    return p;
}

char *mgc_asprintf(mgc_t *mgc, const char *restrict fmt, ...) {
    char *p;
    va_list ap;

    va_start(ap, fmt);
    p = mgc_vasprintf(mgc, fmt, ap);
    va_end(ap);

    return p;
}

/* fd IO */

void mgc_close(void *fdp) {
    while(close(*((int*)fdp)) == -1 && errno == EINTR);
    free(fdp);
}

int mgc_openat(mgc_t *mgc, int fd, const char *path, int oflag, ...) {
    int *fdp;

    if((fdp = malloc(sizeof(int))) == NULL) {
        mgc_add(mgc, NULL, NULL);
        return -1;
    }

    if(oflag & O_CREAT) {
        va_list ap;
        va_start(ap, oflag);
        *fdp = openat(fd, path, oflag, va_arg(ap, mode_t));
        va_end(ap);
    } else {
        *fdp = openat(fd, path, oflag);
    }

    if(*fdp < 0) {
        mgc_add(mgc, NULL, NULL);
        return -1;
    }

    return mgc_add(mgc, fdp, mgc_close) ? *fdp : -1;
}

int mgc_open(mgc_t *mgc, const char *path, int oflag, ...) {
    int fd;
    if(oflag & O_CREAT) {
        va_list ap;
        va_start(ap, oflag);
        fd = mgc_openat(mgc, AT_FDCWD, path, oflag, va_arg(ap, mode_t));
        va_end(ap);
    } else {
        fd = mgc_openat(mgc, AT_FDCWD, path, oflag);
    }
    return fd;
}

/* stdio */

void mgc_fclose(void *stream) {
    while(fclose(stream) == EOF && errno == EINTR);
}

FILE *mgc_fopen(mgc_t *mgc, const char *restrict pathname, const char *restrict mode) {
    return mgc_add(mgc, fopen(pathname, mode), mgc_fclose);
}

FILE *mgc_fdopen(mgc_t *mgc, int fildes, const char *mode) {
    return mgc_add(mgc, fdopen(fildes, mode), mgc_fclose);
}

FILE *mgc_fmemopen(mgc_t *mgc, void *restrict buf, size_t size, const char *mode) {
    return mgc_add(mgc, fmemopen(buf, size, mode), mgc_fclose);
}

FILE *mgc_open_memstream(mgc_t *mgc, char **ptr, size_t *sizeloc) {
    return mgc_add(mgc, open_memstream(ptr, sizeloc), mgc_fclose);
}

#endif /* MGARBAGECOLLECTOR_C */
