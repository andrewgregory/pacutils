#define _GNU_SOURCE

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "mfmt.h"

char *_mfmt_find_unescaped_char(char *haystack, char needle) {
    while(1) {
        haystack = strchrnul(haystack, needle);
        if(*haystack && *(haystack + 1) == needle) { haystack += 2; continue; }
        else { break; }
    }
    return haystack;
}

void _mfmt_brace_dedup(char *str) {
    char *c = str, *end = str + strlen(str);
    while((c = strchr(c, '{'))) {
        memmove(c, c + 1, end - c);
        c++;
    }

    c = str;
    while((c = strchr(c, '}'))) {
        memmove(c, c + 1, end - c);
        c++;
    }
}

mfmt_t *mfmt_parse(const char *tmpl, mfmt_callback_t *cb, void *ctx) {
    mfmt_t *mfmt;
    char *c;

    mfmt = calloc(sizeof(mfmt_t), 1);
    if(mfmt == NULL) { return NULL; }

    mfmt->cb = cb;
    mfmt->ctx = ctx;

    for(c = (char*) tmpl; c && *c; ) {
        mfmt->token_count++;
        if(*c == '{' && *(c + 1) != '{') {
            /* replacement */
            if(!*(c = _mfmt_find_unescaped_char(c + 1, '}'))) {
                errno = EINVAL;
                free(mfmt);
                return NULL;
            } else {
                c++;
            }
        } else {
            /* literal */
            c = _mfmt_find_unescaped_char(c, '{');
        }
    }

    if((mfmt->tokens = calloc(sizeof(mfmt_token_t), mfmt->token_count)) == NULL) {
        free(mfmt);
        return NULL;
    }

    size_t i;
    for(c = (char*) tmpl, i = 0; c && *c; i++) {
        if(*c == '{' && *(c + 1) != '{') {
            /* replacement */
            mfmt_token_callback_t *t = &mfmt->tokens[i].callback;
            char *end = _mfmt_find_unescaped_char(c + 1, '}');
            t->type = MFMT_TOKEN_CALLBACK;
            t->name = strndup(c + 1, end - c - 1);
            c = end + 1;
        } else {
            /* literal */
            char *end = _mfmt_find_unescaped_char(c, '{');
            mfmt_token_literal_t *t = &mfmt->tokens[i].literal;
            t->type = MFMT_TOKEN_LITERAL;
            t->string = strndup(c, end - c);
            _mfmt_brace_dedup(t->string);
            c = end;
        }
    }

    return mfmt;
}

size_t mfmt_printf(mfmt_t *mfmt, void *args, FILE *f) {
    size_t len = 0;
    size_t i;
    for(i = 0; i < mfmt->token_count; i++) {
        mfmt_token_t *t = &mfmt->tokens[i];
        switch(t->base.type) {
            case MFMT_TOKEN_LITERAL:
                len += fputs(t->literal.string, f);
                break;
            case MFMT_TOKEN_CALLBACK:
                len += mfmt->cb(f, &t->callback, mfmt->ctx, args);
                break;
            default:
                errno = EINVAL;
                return 0;
        }
    }
    return len;
}

static size_t _mfmt_printf_close(mfmt_t *mfmt, void *args, FILE *f) {
    if(f) {
        size_t len = mfmt_printf(mfmt, args, f);
        fclose(f);
        return len;
    }
    return -1;
}

size_t mfmt_printd(mfmt_t *mfmt, void *args, int fd) {
    return _mfmt_printf_close(mfmt, args, fdopen(fd, "w"));
}

size_t mfmt_printb(mfmt_t *mfmt, void *args, char *buf, size_t buflen) {
    return _mfmt_printf_close(mfmt, args, fmemopen(buf, buflen, "w"));
}

size_t mfmt_prints(mfmt_t *mfmt, void *args, char **buf, size_t *buflen) {
    return _mfmt_printf_close(mfmt, args, open_memstream(buf, buflen));
}

size_t mfmt_fmt(const char *tmpl, mfmt_val_t *args, FILE *f) {
    mfmt_t *mfmt = mfmt_parse(tmpl, NULL, NULL);
    size_t len;
    for(size_t i = 0; i < mfmt->token_count; i++) {
        mfmt_token_t *t = &mfmt->tokens[i];
        switch(t->base.type) {
            case MFMT_TOKEN_LITERAL:
                len += fputs(t->literal.string, f);
                break;
            case MFMT_TOKEN_CALLBACK:
                /* fprintf(stderr, "token: %s\n", t->callback.name); */
                if(t->callback.name[0]) {
                    for(mfmt_val_t *v = args; v; v++) {
                        /* fprintf(stderr, "val: %s\n", v->name); */
                        if(strcmp(v->name, t->callback.name) == 0) {
                            len += fputs(v->string, f);
                            break;
                        }
                    }
                } else {
                    len += fputs(args->string, f);
                    args++;
                }
                break;
        }
    }
    return len;
}

size_t mfmt_mfmt(mfmt_t *mfmt, mfmt_val_t *args, FILE *f) {
    size_t len;
    for(size_t i = 0; i < mfmt->token_count; i++) {
        mfmt_token_t *t = &mfmt->tokens[i];
        switch(t->base.type) {
            case MFMT_TOKEN_LITERAL:
                len += fputs(t->literal.string, f);
                break;
            case MFMT_TOKEN_CALLBACK:
                /* fprintf(stderr, "token: %s\n", t->callback.name); */
                if(t->callback.name[0]) {
                    for(mfmt_val_t *v = args; v; v++) {
                        /* fprintf(stderr, "val: %s\n", v->name); */
                        if(strcmp(v->name, t->callback.name) == 0) {
                            len += fputs(v->string, f);
                            break;
                        }
                    }
                } else {
                    len += fputs(args->string, f);
                    args++;
                }
                break;
        }
    }
    return len;
}

size_t mfmt_render_int(mfmt_token_callback_t *t, const intmax_t i, FILE *f) {
    (void)t;
    return fprintf(f, "%jd", i);
}

size_t mfmt_render_str(mfmt_token_callback_t *t, const char *str, FILE *f) {
    (void)t;
    return fputs(str, f);
}
