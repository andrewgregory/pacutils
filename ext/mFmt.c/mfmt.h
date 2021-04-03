#include <stdio.h>
#include <stddef.h>
#include <inttypes.h>

typedef enum mfmt_token_type_t {
    MFMT_TOKEN_LITERAL,
    MFMT_TOKEN_CALLBACK,
} mfmt_token_type_t;

typedef struct mfmt_token_literal_t {
    mfmt_token_type_t type;
    char *string;
} mfmt_token_literal_t;

typedef struct mfmt_token_base_t {
    mfmt_token_type_t type;
} mfmt_token_base_t;

typedef struct mfmt_token_callback_t {
    mfmt_token_type_t type;

    size_t position;
    char *name;
    size_t width;
    size_t precision;
    char align;
    char fill;
    char conversion;
    int sign;
} mfmt_token_callback_t;

typedef union mfmt_token_t {
    mfmt_token_base_t base;
    mfmt_token_literal_t literal;
    mfmt_token_callback_t callback;
} mfmt_token_t;

typedef size_t (mfmt_callback_t)(FILE *f, mfmt_token_callback_t *token, void *ctx, void *args);

typedef struct mfmt_t {
    mfmt_callback_t *cb;
    void *ctx;
    size_t token_count;
    mfmt_token_t *tokens;
} mfmt_t;

typedef struct mfmt_val_t {
    const char *name;
    const char *string;
} mfmt_val_t;

mfmt_t *mfmt_parse(const char *tmpl, mfmt_callback_t *cb, void *ctx);
size_t mfmt_printf(mfmt_t *mfmt, void *args, FILE *f);
size_t mfmt_printd(mfmt_t *mfmt, void *args, int fd);
size_t mfmt_printb(mfmt_t *mfmt, void *args, char *buf, size_t buflen);
size_t mfmt_prints(mfmt_t *mfmt, void *args, char **buf, size_t *buflen);
void mfmt_free(mfmt_t *mfmt);

size_t mfmt_render_int(mfmt_token_callback_t *token, intmax_t i, FILE *f);
size_t mfmt_render_uint(mfmt_token_callback_t *token, uintmax_t i, FILE *f);
size_t mfmt_render_str(mfmt_token_callback_t *token, const char *str, FILE *f);

size_t mfmt_formatf(const char *tmpl, mfmt_callback_t *cb, void *ctx, FILE *f);
size_t mfmt_formatd(const char *tmpl, mfmt_callback_t *cb, void *ctx, int fd);
size_t mfmt_formatb(const char *tmpl, mfmt_callback_t *cb, void *ctx, char *buf, size_t buflen);
size_t mfmt_formats(const char *tmpl, mfmt_callback_t *cb, void *ctx, char **buf);
