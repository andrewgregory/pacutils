/*
 * Copyright 2021 Andrew Gregory <andrew.gregory.8@gmail.com>
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

#include <errno.h>
#include <string.h>

#include "../../ext/mFmt.c/mfmt.h"

#include <pacutils/util.h>

#include "info.h"

typedef enum field_t {
  FILENAME,
  NAME,
  BASE,
  DESCRIPTION,
  VERSION,
  ORIGIN,
  REASON,
  LICENSE,
  GROUP,

  DEPENDS,
  OPTDEPENDS,
  CONFLICTS,
  PROVIDES,
  REPLACES,
  REQUIREDBY,

  DELTAS,
  FILES,
  BACKUP,
  DB,
  VALIDATION,
  URL,
  BUILDDATE,
  INSTALLDATE,
  PACKAGER,
  MD5SUM,
  SHA256SUM,
  ARCH,
  SIZE,
  ISIZE,
  BASE64SIG,

  UNKNOWN,
} field_t;

static struct field_map_t {
  char *input;
  field_t field;
} field_map[] = {
  {"filename", FILENAME},
  {"name", NAME},
  {"base", BASE},
  {"description", DESCRIPTION},
  {"version", VERSION},

  {"license", LICENSE},
  {"group", GROUP},
  {"groups", GROUP},

  {"depends", DEPENDS},
  {"optdepends", OPTDEPENDS},
  {"conflicts", CONFLICTS},
  {"provides", PROVIDES},
  {"replaces", REPLACES},
  {"requiredby", REQUIREDBY},

  {"url", URL},
  {"builddate", BUILDDATE},
  {"installdate", INSTALLDATE},
  {"packager", PACKAGER},
  {"md5sum", MD5SUM},
  {"sha256sum", SHA256SUM},
  {"arch", ARCH},
  {"size", SIZE},
  {"isize", ISIZE},
  {"base64sig", BASE64SIG},

  {NULL, 0}
};

static field_t _pu_info_lookup_field(const char *name) {
  struct field_map_t *m;
  for(m = field_map; m->input; m++) {
    if(strcmp(name, m->input) == 0) { return m->field; }
  }
  return UNKNOWN;
}

size_t _pu_info_print_str(mfmt_token_callback_t *t, const char *str, FILE *f) {
  return mfmt_render_str(t, str ? str : "NULL", f);
}

size_t _pu_info_print_size(mfmt_token_callback_t *t, const off_t s, FILE *f) {
  if(s) {
    char hrsize[50];
    if(t->conversion == 'd') {
      snprintf(hrsize, 50, "%lld", (long long)s);
    } else {
      pu_hr_size(s, hrsize);
    }
    return mfmt_render_str(t, hrsize, f);
  } else {
    return mfmt_render_str(t, "NULL", f);
  }
}

size_t _pu_info_print_strlist(mfmt_token_callback_t *t, alpm_list_t *l, FILE *f) {
  if(l) {
    size_t len = 0;
    while(l) {
      len += mfmt_render_str(t, l->data, f) + 1;
      fputc('\n', f);
      l = l->next;
    }
    return len;
  } else {
    return mfmt_render_str(t, "NULL", f);
  }
}

size_t _pu_info_print_deplist(mfmt_token_callback_t *t, alpm_list_t *l, FILE *f) {
  if(l) {
    size_t len = 0;
    while(l) {
      char *s = alpm_dep_compute_string(l->data);
      len += mfmt_render_str(t, s, f) + 1;
      fputc('\n', f);
      l = l->next;
      free(s);
    }
    return len;
  } else {
    return mfmt_render_str(t, "NULL", f);
  }
}

size_t _pu_info_print_timestamp(mfmt_token_callback_t *t, const alpm_time_t s, FILE *f) {
  if(s) {
    char datestr[50] = "";
    if(strftime(datestr, 50, " %c", localtime(&s)) == 0) { return 0; }
    return mfmt_render_str(t, datestr + 1, f);
  } else {
    return mfmt_render_str(t, "NULL", f);
  }
}

size_t _pu_info_process_token(FILE *f, mfmt_token_callback_t *t, void *ctx, void *arg) {
  alpm_pkg_t *p = arg;
  (void)ctx;
  switch(_pu_info_lookup_field(t->name)) {
    case NAME:        return _pu_info_print_str(t, alpm_pkg_get_name(p), f);
    case DESCRIPTION: return _pu_info_print_str(t, alpm_pkg_get_desc(p), f);
    case PACKAGER:    return _pu_info_print_str(t, alpm_pkg_get_packager(p), f);
    case MD5SUM:      return _pu_info_print_str(t, alpm_pkg_get_md5sum(p), f);
    case FILENAME:    return _pu_info_print_str(t, alpm_pkg_get_filename(p), f);
    case BASE:        return _pu_info_print_str(t, alpm_pkg_get_base(p), f);
    case VERSION:     return _pu_info_print_str(t, alpm_pkg_get_version(p), f);
    case URL:         return _pu_info_print_str(t, alpm_pkg_get_url(p), f);
    case SHA256SUM:   return _pu_info_print_str(t, alpm_pkg_get_sha256sum(p), f);
    case ARCH:        return _pu_info_print_str(t, alpm_pkg_get_arch(p), f);
    case BASE64SIG:   return _pu_info_print_str(t, alpm_pkg_get_base64_sig(p), f);

    case SIZE:  return _pu_info_print_size(t, alpm_pkg_get_size(p), f);
    case ISIZE: return _pu_info_print_size(t, alpm_pkg_get_isize(p), f);

    case BUILDDATE:   return _pu_info_print_timestamp(t, alpm_pkg_get_builddate(p), f);
    case INSTALLDATE: return _pu_info_print_timestamp(t, alpm_pkg_get_installdate(p), f);

    case DEPENDS:    return _pu_info_print_deplist(t, alpm_pkg_get_depends(p), f);
    case OPTDEPENDS: return _pu_info_print_deplist(t, alpm_pkg_get_optdepends(p), f);
    case CONFLICTS:  return _pu_info_print_deplist(t, alpm_pkg_get_conflicts(p), f);
    case PROVIDES:   return _pu_info_print_deplist(t, alpm_pkg_get_provides(p), f);
    case REPLACES:   return _pu_info_print_deplist(t, alpm_pkg_get_replaces(p), f);
    case REQUIREDBY: {
                       alpm_list_t *rb = alpm_pkg_compute_requiredby(p);
                       size_t len = _pu_info_print_strlist(t, rb, f);
                       FREELIST(rb);
                       return len;
                     }

    default:          errno = EINVAL; return 0;
  }
}

size_t pu_info_print_pkg(const char *format, alpm_pkg_t *pkg) {
  alpm_list_t l = {
    .data = pkg,
    .next = NULL,
  };
  return pu_info_print_pkgs(format, &l);
}

size_t pu_info_print_pkgs(const char *format, alpm_list_t *pkgs) {
  mfmt_t *mfmt = mfmt_parse(format, _pu_info_process_token, NULL);
  size_t len = 0;
  if(mfmt == NULL) {
    return 0;
  }
  for(alpm_list_t *i = pkgs; i; i = i->next) {
    size_t plen = mfmt_printf(mfmt, i->data, stdout);
    if(plen == 0) { return 0; }
    len += plen;
  }
  return len;
}

/* vim: set ts=2 sw=2 et: */
