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

#ifndef PACUTILS_DEPENDS_H
#define PACUTILS_DEPENDS_H

#include <alpm.h>

int pu_provision_satisfies_dep(alpm_depend_t *provision, alpm_depend_t *dep);
int pu_pkg_satisfies_dep(alpm_pkg_t *pkg, alpm_depend_t *dep);
int pu_pkg_depends_on(alpm_pkg_t *pkg, alpm_pkg_t *dpkg);
int pu_pkg_optdepends_on(alpm_pkg_t *pkg, alpm_pkg_t *dpkg);
int pu_pkg_checkdepends_on(alpm_pkg_t *pkg, alpm_pkg_t *dpkg);
int pu_pkg_makedepends_on(alpm_pkg_t *pkg, alpm_pkg_t *dpkg);

int pu_pkg_find_requiredby(alpm_pkg_t *pkg, alpm_list_t *pkgs,
    alpm_list_t **ret);
int pu_pkg_find_optionalfor(alpm_pkg_t *pkg, alpm_list_t *pkgs,
    alpm_list_t **ret);
int pu_pkg_find_makedepfor(alpm_pkg_t *pkg, alpm_list_t *pkgs,
    alpm_list_t **ret);
int pu_pkg_find_checkdepfor(alpm_pkg_t *pkg, alpm_list_t *pkgs,
    alpm_list_t **ret);

alpm_pkg_t *pu_pkglist_find_dep_satisfier(alpm_list_t *pkgs,
    alpm_depend_t *dep);
alpm_pkg_t *pu_db_find_dep_satisfier(alpm_db_t *db, alpm_depend_t *dep);
alpm_pkg_t *pu_dblist_find_dep_satisfier(alpm_list_t *dbs, alpm_depend_t *dep);

#endif /* PACUTILS_DEPENDS_H */

/* vim: set ts=2 sw=2 et: */
