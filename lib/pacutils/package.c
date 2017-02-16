/*
 * Copyright 2017 Andrew Gregory <andrew.gregory.8@gmail.com>
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

#include "depend.h"
#include "package.h"

int pu_package_satisfies_dep(alpm_pkg_t *pkg, alpm_depend_t *dep) {
	if(pu_depend_satisfied_by(dep,
				alpm_pkg_get_name(pkg), alpm_pkg_get_version(pkg))) {
		return 1;
	} else {
		alpm_list_t *i;
		for(i = alpm_pkg_get_provides(pkg); i; i = i->next) {
			alpm_depend_t *p = i->data;
			if(pu_depend_satisfied_by(dep, p->name, p->version)) {
				return 1;
			}
		}
	}
	return 0;
}

int pu_pkg_depends_on(alpm_pkg_t *pkg, alpm_pkg_t *dep)
{
	alpm_list_t *i;
	for(i = alpm_pkg_get_depends(pkg); i; i = i->next) {
		if(pu_package_satisfies_dep(dep, i->data)) {
			return 1;
		}
	}
	return 0;
}

int pu_pkg_optdepends_on(alpm_pkg_t *pkg, alpm_pkg_t *dep)
{
	alpm_list_t *i;
	for(i = alpm_pkg_get_depends(pkg); i; i = i->next) {
		if(pu_package_satisfies_dep(dep, i->data)) {
			return 1;
		}
	}
	return 0;
}

/* vim: set ts=2 sw=2 et: */
