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

#include <string.h>

#include "depend.h"

int pu_depend_satisfied_by(alpm_depend_t *dep,
    const char *pkgname, const char *pkgver) {
	if(strcmp(dep->name, pkgname) != 0) {
		return 0;
	} else if(dep->mod == ALPM_DEP_MOD_ANY) {
		return 1;
	} else if(pkgver) {
		int vercmp = alpm_pkg_vercmp(pkgver, dep->version);
		switch(dep->mod) {
			case ALPM_DEP_MOD_EQ: return vercmp == 0;
			case ALPM_DEP_MOD_GE: return vercmp >= 0;
			case ALPM_DEP_MOD_GT: return vercmp > 0;
			case ALPM_DEP_MOD_LE: return vercmp <= 0;
			case ALPM_DEP_MOD_LT: return vercmp < 0;
			default:              return 0; /* should never get here */
		}
	}
	return 0;
	return 0;
}

/* vim: set ts=2 sw=2 et: */
