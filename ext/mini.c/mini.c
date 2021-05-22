/*
 * Copyright 2014 Andrew Gregory <andrew.gregory.8@gmail.com>
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
 * Project URL: http://github.com/andrewgregory/mINI.c
 */

#ifndef MINI_C
#define MINI_C

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mini.h"

mini_t *mini_finit(FILE *stream) {
    mini_t *mini;

    if((mini = calloc(sizeof(mini_t), 1)) == NULL) { return NULL; }

    mini->_buf_size = MINI_BUFFER_SIZE;
    mini->_buf = malloc(mini->_buf_size);
    if(!mini->_buf) { mini_free(mini); return NULL; }

    mini->stream = stream;

    return mini;
}

mini_t *mini_init(const char *path) {
    FILE *stream;
    mini_t *m;
    if(!(stream = fopen(path, "r"))) { return NULL; }
    if(!(m = mini_finit(stream))) { fclose(stream); return NULL; };
    m->_free_stream = 1;
    return m;
}

void mini_free(mini_t *mini) {
    if(mini == NULL) { return; }
    free(mini->_buf);
    free(mini->section);
    if(mini->stream && mini->_free_stream) { fclose(mini->stream); }
    free(mini);
}

static inline int _mini_isspace(int c) {
    return c == ' ' || c == '\n' || c == '\t'
        || c == '\r' || c == '\f' || c == '\v';
}

static size_t _mini_strtrim(char *str, size_t len) {
    char *start = str, *end = str + len - 1;
    size_t newlen;

    if(!(str && *str)) { return 0; }

    while(*start && _mini_isspace((int) *start)) { start++; }
    while(end > start && _mini_isspace((int) *end)) { end--; }

    *(++end) = '\0';
    newlen = (size_t)(end - start);
    memmove(str, start, newlen + 1);

    return newlen;
}

/* fgetc wrapper that retries on EINTR */
static int _mini_fgetc(FILE *stream) {
    int c, errno_orig = errno;
    do { errno = 0; c = fgetc(stream); } while(c == EOF && errno == EINTR);
    if(c != EOF || feof(stream) ) { errno = errno_orig; }
    return c;
}

/* fgets replacement to handle signal interrupts */
static char *_mini_fgets(char *buf, size_t size, FILE *stream) {
    char *s = buf;
    int c;

    if(size == 1) {
        *s = '\0';
        return buf;
    } else if(size < 1) {
        return NULL;
    }

    do {
        c = _mini_fgetc(stream);
        if(c != EOF) { *(s++) = (char) c; }
    } while(--size > 1 && c != EOF && c != '\n');

    if(s == buf || ( c == EOF && !feof(stream) )) {
        return NULL;
    } else {
        *s = '\0';
        return buf;
    }
}

mini_t *mini_next(mini_t *mini) {
    char *c = NULL;
    size_t buflen = 0;

    mini->key = NULL;
    mini->value = NULL;

    if(_mini_fgets(mini->_buf, mini->_buf_size, mini->stream) == NULL) {
        mini->eof = feof(mini->stream);
        return NULL;
    }

    mini->lineno++;

    buflen = strlen(mini->_buf);
    if(mini->_buf[buflen - 1] != '\n' && !feof(mini->stream)) {
        errno = EOVERFLOW;
        return NULL;
    }

    if((c = strchr(mini->_buf, '#'))) {
        *c = '\0';
        buflen = (size_t)(c - mini->_buf);
    }

    buflen = _mini_strtrim(mini->_buf, buflen);

    if(buflen == 0) { return mini_next(mini); }

    if(mini->_buf[0] == '[' && mini->_buf[buflen - 1] == ']') {
        free(mini->section);
        mini->_buf[buflen - 1] = '\0';
        if((mini->section = malloc(buflen - 1)) == NULL) { return NULL; }
        memcpy(mini->section, mini->_buf + 1, buflen - 1);
    } else {
        mini->key = mini->_buf;
        if((c = strchr(mini->_buf, '='))) {
            *c = '\0';
            mini->value = c + 1;
            _mini_strtrim(mini->key, (size_t)(c - mini->_buf));
            _mini_strtrim(mini->value, buflen - (size_t)(mini->value - mini->_buf));
        }
    }

    return mini;
}

mini_t *mini_lookup_key(mini_t *mini, const char *section, const char *key) {
    int in_section = (section == NULL);

    rewind(mini->stream);
    free(mini->section);
    mini->section = NULL;
    mini->key = NULL;
    mini->value = NULL;
    mini->lineno = 0;
    mini->eof = 0;

    if(!key) { return NULL; }

    while(mini_next(mini)) {
        if(!mini->key) {
            /* starting a new section */
            in_section = (strcmp(mini->section, section) == 0);
        } else if(in_section && strcmp(mini->key, key) == 0) {
            return mini;
        }
    }

    return NULL;
}

int mini_fparse_cb(FILE *stream, mini_cb_t cb, void *data) {
    mini_t *mini = mini_finit(stream);
    int ret = 0;

    if(!mini) {
        return cb(0, NULL, NULL, NULL, data);
    }

    while(ret == 0 && mini_next(mini)) {
        ret = cb(mini->lineno, mini->section, mini->key, mini->value, data);
    }
    if(ret == 0 && !mini->eof) {
        ret = cb(0, NULL, NULL, NULL, data);
    }

    mini_free(mini);
    return ret;
}

int mini_parse_cb(const char *path, mini_cb_t cb, void *data) {
    FILE *stream = fopen(path, "r");
    int ret;
    if(stream) {
        ret = mini_fparse_cb(stream, cb, data);
        fclose(stream);
    } else {
        ret = cb(0, NULL, NULL, NULL, data);
    }
    return ret;
}

#endif /* MINI_C */
