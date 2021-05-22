/*
 * Copyright 2012-2020 Andrew Gregory <andrew.gregory.8@gmail.com>
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

#include <string.h>

#include "depends.h"

typedef enum pu_deptype_t {
  PU_DEPTYPE_DEPEND   = (1 << 0),
  PU_DEPTYPE_OPTIONAL = (1 << 1),
  PU_DEPTYPE_MAKE     = (1 << 2),
  PU_DEPTYPE_CHECK    = (1 << 3),
} pu_deptype_t;

static int pu_pkgver_satisfies_dep(const char *ver, alpm_depend_t *dep) {
  switch (dep->mod) {
    case ALPM_DEP_MOD_ANY:
      return 1;
    case ALPM_DEP_MOD_EQ:
      return alpm_pkg_vercmp(ver, dep->version) == 0;
    case ALPM_DEP_MOD_LE:
      return alpm_pkg_vercmp(ver, dep->version) <= 0;
    case ALPM_DEP_MOD_GE:
      return alpm_pkg_vercmp(ver, dep->version) >= 0;
    case ALPM_DEP_MOD_LT:
      return alpm_pkg_vercmp(ver, dep->version) < 0;
    case ALPM_DEP_MOD_GT:
      return alpm_pkg_vercmp(ver, dep->version) > 0;
  }
  return 0; /* should never reach this */
}

int pu_provision_satisfies_dep(alpm_depend_t *provision, alpm_depend_t *dep) {
  /* alpm exposes the alpm_depend_t structure, but does not provide a method
   * for filling in the name_hash, only rely on it if both arguments have it
   * filled in */
  return (provision->name_hash == 0 || dep->name_hash == 0
          || provision->name_hash == dep->name_hash)
      && strcmp(provision->name, dep->name) == 0
      && pu_pkgver_satisfies_dep(provision->version, dep);
}

int pu_pkg_satisfies_dep(alpm_pkg_t *pkg, alpm_depend_t *dep) {
  alpm_list_t *p;
  if (strcmp(alpm_pkg_get_name(pkg), dep->name) == 0 &&
      pu_pkgver_satisfies_dep(alpm_pkg_get_version(pkg), dep)) {
    return 1;
  }
  for (p = alpm_pkg_get_provides(pkg); p; p = p->next) {
    if (pu_provision_satisfies_dep(p->data, dep)) {
      return 1;
    }
  }
  return 0;
}

static int _pu_pkg_satisfies_deplist(alpm_pkg_t *pkg, alpm_list_t *deps) {
  while (deps) {
    if (pu_pkg_satisfies_dep(pkg, deps->data)) { return 1; }
    deps = deps->next;
  }
  return 0;
}

int pu_pkg_depends_on(alpm_pkg_t *pkg, alpm_pkg_t *dpkg) {
  return _pu_pkg_satisfies_deplist(dpkg, alpm_pkg_get_depends(pkg));
}

int pu_pkg_optdepends_on(alpm_pkg_t *pkg, alpm_pkg_t *dpkg) {
  return _pu_pkg_satisfies_deplist(dpkg, alpm_pkg_get_optdepends(pkg));
}

int pu_pkg_checkdepends_on(alpm_pkg_t *pkg, alpm_pkg_t *dpkg) {
  return _pu_pkg_satisfies_deplist(dpkg, alpm_pkg_get_checkdepends(pkg));
}

int pu_pkg_makedepends_on(alpm_pkg_t *pkg, alpm_pkg_t *dpkg) {
  return _pu_pkg_satisfies_deplist(dpkg, alpm_pkg_get_makedepends(pkg));
}

static int pu_pkg_find_reversedeps(alpm_pkg_t *pkg, int type,
    alpm_list_t *pkgs, alpm_list_t **ret) {
  alpm_list_t *h;
  for (h = pkgs; h; h = h->next) {
    if ( (type & PU_DEPTYPE_DEPEND   && pu_pkg_depends_on(h->data, pkg))
        || (type & PU_DEPTYPE_OPTIONAL && pu_pkg_optdepends_on(h->data, pkg))
        || (type & PU_DEPTYPE_CHECK    && pu_pkg_checkdepends_on(h->data, pkg))
        || (type & PU_DEPTYPE_MAKE     && pu_pkg_makedepends_on(h->data, pkg)) ) {
      if (alpm_list_append(ret, h->data) == NULL) {
        return -1;
      }
    }
  }
  return 0;
}

int pu_pkg_find_requiredby(alpm_pkg_t *pkg, alpm_list_t *pkgs,
    alpm_list_t **ret) {
  return pu_pkg_find_reversedeps(pkg, PU_DEPTYPE_DEPEND, pkgs, ret);
}

int pu_pkg_find_optionalfor(alpm_pkg_t *pkg, alpm_list_t *pkgs,
    alpm_list_t **ret) {
  return pu_pkg_find_reversedeps(pkg, PU_DEPTYPE_OPTIONAL, pkgs, ret);
}

int pu_pkg_find_makedepfor(alpm_pkg_t *pkg, alpm_list_t *pkgs,
    alpm_list_t **ret) {
  return pu_pkg_find_reversedeps(pkg, PU_DEPTYPE_MAKE, pkgs, ret);
}

int pu_pkg_find_checkdepfor(alpm_pkg_t *pkg, alpm_list_t *pkgs,
    alpm_list_t **ret) {
  return pu_pkg_find_reversedeps(pkg, PU_DEPTYPE_CHECK, pkgs, ret);
}

alpm_pkg_t *pu_pkglist_find_dep_satisfier(alpm_list_t *pkgs,
    alpm_depend_t *dep) {
  alpm_list_t *p;
  for (p = pkgs; p; p = p->next) {
    if (pu_pkg_satisfies_dep(p->data, dep)) { return p->data; }
  }
  return NULL;
}

alpm_pkg_t *pu_db_find_dep_satisfier(alpm_db_t *db, alpm_depend_t *dep) {
  alpm_pkg_t *pkg = alpm_db_get_pkg(db, dep->name);
  if (pkg && pu_pkg_satisfies_dep(pkg, dep)) { return pkg; }
  return pu_pkglist_find_dep_satisfier(alpm_db_get_pkgcache(db), dep);
}

alpm_pkg_t *pu_dblist_find_dep_satisfier(alpm_list_t *dbs, alpm_depend_t *dep) {
  alpm_list_t *d;
  for (d = dbs; d; d = d->next) {
    alpm_pkg_t *pkg = alpm_db_get_pkg(d->data, dep->name);
    if (pkg && pu_pkg_satisfies_dep(pkg, dep)) { return pkg; }
  }
  for (d = dbs; d; d = d->next) {
    alpm_pkg_t *pkg = pu_pkglist_find_dep_satisfier(alpm_db_get_pkgcache(d->data),
            dep);
    if (pkg) { return pkg; }
  }
  return NULL;
}

/* vim: set ts=2 sw=2 noet: */
