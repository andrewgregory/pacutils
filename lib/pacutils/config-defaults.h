/*
 * Copyright 2016 Andrew Gregory <andrew.gregory.8@gmail.com>
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

#ifndef PACUTILS_CONFIG_DEFAULTS_H
#define PACUTILS_CONFIG_DEFAULTS_H

#define BASEVER "0.4.0"

#ifdef GITVER
#define BUILDVER BASEVER "+" GITVER
#else
#define BUILDVER BASEVER
#endif

/* duplicates default configuration settings from lib/Makefile in order to
 * allow compilation without make and to provide defaults for analyzers */

#ifndef PREFIX
#define PREFIX "/usr/local"
#endif

#ifndef LOCALSTATEDIR
#define LOCALSTATEDIR PREFIX "/var"
#endif

#ifndef SYSCONFDIR
#define SYSCONFDIR PREFIX "/etc"
#endif

#ifndef CACHEDIR
#define CACHEDIR LOCALSTATEDIR "/cache/pacman/pkg"
#endif

#ifndef DBEXT
#define DBEXT ".db"
#endif

#ifndef DBPATH
#define DBPATH LOCALSTATEDIR "/lib/pacman"
#endif

#ifndef GPGDIR
#define GPGDIR SYSCONFDIR "/pacman.d/gnupg"
#endif

#ifndef HOOKDIR
#define HOOKDIR SYSCONFDIR "/pacman.d/hooks"
#endif

#ifndef LOGFILE
#define LOGFILE LOCALSTATEDIR "/log/pacman.log"
#endif

#ifndef ROOTDIR
#define ROOTDIR "/"
#endif

#endif /* PACUTILS_CONFIG_DEFAULTS_H */

/* vim: set ts=2 sw=2 et: */
