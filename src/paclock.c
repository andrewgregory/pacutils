/*
 * Copyright 2019 Andrew Gregory <andrew.gregory.8@gmail.com>
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
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <sys/wait.h>
#include <unistd.h>

#include <alpm.h>
#include <pacutils.h>

#include "config-defaults.h"

const char *myname = "paclock", *myver = BUILDVER;

enum longopt_flags {
	FLAG_CONFIG = 1000,
	FLAG_DBPATH,
	FLAG_HELP,
	FLAG_KEY,
	FLAG_LOCKFILE,
	FLAG_ROOT,
	FLAG_SYSROOT,
	FLAG_VERSION,
};

enum action {
	ACTION_LOCK,
	ACTION_UNLOCK,
	ACTION_PRINT,
	ACTION_RUN,
};

char *config_file = PACMANCONF, *dbpath = NULL, *root = NULL, *sysroot = NULL;
char *key = NULL, *lockfile = NULL;
int nocheck = 0, enoent_ok = 0, action = ACTION_LOCK;

void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
#define hputs(s) fputs(s"\n", stream);
	hputs("paclock - lock/unlock the alpm database");
	hputs("usage: paclock [options]");
	hputs("");
	hputs("options:");
	hputs("   --config=<path>    set an alternate configuration file");
	hputs("   --dbpath=<path>    set an alternate database location");
	hputs("   --root=<path>      set an alternate installation root");
	hputs("   --sysroot=<path>   set an alternate installation system root");
	hputs("   --key=<id>         set the lock identity key");
	hputs("   --lockfile=<path>  set the lock file path");
	hputs("   --lock             establish a new lock (default)");
	hputs("   --unlock           remove an existing lock");
	hputs("   --run              run a command with the database locked");
	hputs("   --print            print the lock file path exit");
	hputs("   --no-check-keys    skip check for matching lock file key before unlocking");
	hputs("   --enoent-ok        ignore unlock errors due to a missing lock file");
#undef hputs
	exit(ret);
}

void parse_opts(int argc, char **argv)
{
	int c;

	char *short_opts = "+";
	struct option long_opts[] = {
		{ "config"        , required_argument , NULL       , FLAG_CONFIG       } ,
		{ "dbpath"        , required_argument , NULL       , FLAG_DBPATH       } ,
		{ "root"          , required_argument , NULL       , FLAG_ROOT         } ,
		{ "sysroot"       , required_argument , NULL       , FLAG_SYSROOT      } ,

		{ "key"           , required_argument , NULL       , FLAG_KEY          } ,
		{ "lockfile"      , required_argument , NULL       , FLAG_LOCKFILE     } ,

		{ "no-check-keys" , no_argument       , &nocheck   , 1                 } ,

		{ "enoent-ok"     , no_argument       , &enoent_ok , 1                 } ,

		{ "lock"          , no_argument       , &action    , ACTION_LOCK       } ,
		{ "unlock"        , no_argument       , &action    , ACTION_UNLOCK     } ,
		{ "print"         , no_argument       , &action    , ACTION_PRINT      } ,
		{ "run"           , no_argument       , &action    , ACTION_RUN        } ,

		{ "help"          , no_argument       , NULL       , FLAG_HELP         } ,
		{ "version"       , no_argument       , NULL       , FLAG_VERSION      } ,

		{ 0, 0, 0, 0 },
	};

	while((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch(c) {
			case FLAG_HELP:
				usage(0);
				break;
			case FLAG_VERSION:
				pu_print_version(myname, myver);
				exit(0);
				break;

			/* general options */
			case FLAG_KEY:
				key = optarg;
				break;
			case FLAG_LOCKFILE:
				lockfile = strdup(optarg);
				break;
			case FLAG_SYSROOT:
				sysroot = optarg;
				break;
			case FLAG_CONFIG:
				config_file = optarg;
				break;
			case FLAG_DBPATH:
				dbpath = optarg;
				break;
			case FLAG_ROOT:
				root = optarg;
				break;

			case '?':
			case ':':
				usage(1);
				break;
		}
	}
}

int create_lock(void) {
	int fd = -1;
	do {
		fd = open(lockfile, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0000);
	} while(fd == -1 && errno == EINTR);
	if(fd == -1) {
		pu_ui_error("could not create lock file (%s)", strerror(errno));
		return 1;
	}
	if(dprintf(fd, "%s", key) < 0) {
		pu_ui_error("could not write key to lock file (%s)", strerror(errno));
		unlink(lockfile);
		close(fd);
		return 1;
	}
	close(fd);
	return 0;
}

int check_key(void) {
	char *file_key = NULL;
	size_t file_key_len = 0;
	FILE *f;

	if((f = fopen(lockfile, "r")) == NULL) {
		if(errno == ENOENT && enoent_ok) {
			return 0;
		}
		pu_ui_error("could not open '%s' for reading (%s)", lockfile, strerror(errno));
		return 1;
	}
	if(getdelim(&file_key, &file_key_len, '\0', f) == -1) {
		pu_ui_error("could not read key from '%s' (%s)", lockfile, strerror(errno));
		fclose(f);
		return 1;
	}
	fclose(f);

	if(strcmp(file_key, key) != 0) {
		pu_ui_error("lock keys do not match: '%s' != '%s'", file_key, key);
		free(file_key);
		return 1;
	}
	free(file_key);

	return 0;
}

int remove_lock(void) {
	if(!nocheck && check_key() != 0) {
		return 1;
	}

	if(unlink(lockfile) != 0) {
		if(errno == ENOENT && enoent_ok) {
			return 0;
		}
		pu_ui_error("unable to remove lock file '%s' (%s)", lockfile, strerror(errno));
		return 1;
	}

	return 0;
}

int run_cmd(char **argv) {
	int pid, ret;
	if(create_lock() != 0) { return 1; }
	switch((pid = fork())) {
		case -1:
			remove_lock();
			pu_ui_error("unable to fork process (%s)", strerror(errno));
			remove_lock(); /* command didn't run, the lock can safely be removed */
			return 1;
		case 0:
			/* child */
			execvp(argv[0], argv);
			pu_ui_error("unable to exec '%s' (%s)", argv[0], strerror(errno));
			remove_lock(); /* command didn't run, the lock can safely be removed */
			exit(1);
		default:
			/* parent */
			while(waitpid(pid, &ret, 0) == -1 && errno == EINTR);
			/* if something went wrong, leave the lock to indicate an error */
			if(WEXITSTATUS(ret) != 0) { return WEXITSTATUS(ret); }
			return remove_lock();
	}
}

int main(int argc, char *argv[]) {
	parse_opts(argc, argv);

	if(key == NULL) {
		char host[HOST_NAME_MAX];
		if(gethostname(host, HOST_NAME_MAX) == -1 ||
				(key = pu_asprintf("%s:%d", host, getppid())) == NULL) {
			pu_ui_error("unable to set default key (%s)", strerror(errno));
			return 1;
		}
	}

	if(lockfile == NULL) {
		pu_config_t *config;
		alpm_handle_t *handle;

		if((config = pu_config_new()) == NULL) {
			pu_ui_error("unable to set lockfile (%s)", strerror(errno));
			return 1;
		}

		if(root && (config->rootdir = strdup(root)) == NULL) {
			pu_ui_error("unable to set lockfile (%s)", strerror(errno));
			return 1;
		}
		if(dbpath && (config->dbpath = strdup(dbpath)) == NULL) {
			pu_ui_error("unable to set lockfile (%s)", strerror(errno));
			return 1;
		}

		if(!pu_ui_config_load_sysroot(config, config_file, sysroot)) {
			pu_ui_error("unable to set lockfile (could not parse '%s')", config_file);
			return 1;
		}

		if(!(handle = pu_initialize_handle_from_config(config))) {
			pu_ui_error("failed to initialize alpm");
			return 1;
		}

		if((lockfile = strdup(alpm_option_get_lockfile(handle))) == NULL) {
			pu_ui_error("unable to set lockfile (%s)", strerror(errno));
			alpm_release(handle);
			return 1;
		}
	}

	switch(action) {
		case ACTION_LOCK: return create_lock();
		case ACTION_UNLOCK: return remove_lock();
		case ACTION_RUN: return run_cmd(argv + optind);
		case ACTION_PRINT: fputs(lockfile, stdout); return 0;
	}

	free(lockfile);

	return 1;
}
