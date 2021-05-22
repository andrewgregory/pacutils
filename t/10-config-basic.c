#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "pacutils.h"

#include "pacutils_test.h"

FILE *f = NULL;
pu_config_t *config = NULL;
pu_config_reader_t *reader = NULL;

void cleanup(void) {
	fclose(f);
	pu_config_free(config);
	pu_config_reader_free(reader);
}

char buf[] =
"\n"
"[options]\n"
"#RootDir = /root1\n"
"RootDir = /root2\n"
"RootDir = /root3\n"
"DBPath = /dbpath1/ \n"
"DBPath = /dbpath2/ \n"
"CacheDir=/cachedir\n"
"HookDir=/hookdir1\n"
"HookDir=/hookdir2\n"
"GPGDir =gpgdir\n"
"GPGDir =gpgdir2\n"
"LogFile= /logfile #this is a comment\n"
"LogFile= /logfile2\n"
" HoldPkg = holdpkga holdpkgb \n"
"IgnorePkg = ignorepkga\n"
"IgnorePkg = ignorepkgb\n"
" IgnoreGroup = ignoregroupa ignoregroupb \n"
"Architecture = i686\n"
"Architecture = x86_64\n"
"ParallelDownloads = 3\n"
"XferCommand = xcommand\n"
"XferCommand = xcommand2\n"
"NoUpgrade = /tmp/noupgrade*\n"
"NoExtract = /tmp/noextract*\n"
"CleanMethod = KeepInstalled KeepCurrent\n"
"UseSyslog\n"
"Color\n"
"CheckSpace\n"
"VerbosePkgLists\n"
"ILoveCandy\n"
"SigLevel = Never\n"
"\n"
"[core]\n"
"Server = $repo:$arch\n"
"";

#define is_list_exhausted(l, name) do { \
	tap_ok(l == NULL, name " exhausted"); \
	if(l) { \
		tap_diag("remaining elements:"); \
		while(l) { \
			tap_diag(l->data); \
			l = alpm_list_next(l); \
		} \
	} \
} while(0)

#define is_str_list(l, str, desc) do { \
	if(l) { \
		tap_is_str(l->data, str, desc); \
		l = alpm_list_next(l); \
	} else { \
		tap_ok(l != NULL, desc); \
	} \
} while(0)

#define sig_diag(sl, s) if(sl & s) { tap_diag("              %s", #s); }
#define sig_dump(sl) \
	do { \
		sig_diag(sl, ALPM_SIG_USE_DEFAULT); \
		sig_diag(sl, ALPM_SIG_PACKAGE); \
		sig_diag(sl, ALPM_SIG_PACKAGE_OPTIONAL); \
		sig_diag(sl, ALPM_SIG_PACKAGE_UNKNOWN_OK); \
		sig_diag(sl, ALPM_SIG_PACKAGE_MARGINAL_OK); \
		sig_diag(sl, ALPM_SIG_DATABASE); \
		sig_diag(sl, ALPM_SIG_DATABASE_OPTIONAL); \
		sig_diag(sl, ALPM_SIG_DATABASE_UNKNOWN_OK); \
		sig_diag(sl, ALPM_SIG_DATABASE_MARGINAL_OK); \
	} while(0)

#define is_siglevel(g, e, ...) \
	do { \
		if(!tap_ok(g == e, __VA_ARGS__)) { \
			tap_diag("   expected:"); \
			sig_dump(e); \
			tap_diag("        got:"); \
			sig_dump(g); \
		} \
	} while(0)

int main(void) {
	alpm_list_t *i;
	pu_repo_t *repo;

	ASSERT(atexit(cleanup) == 0);
	ASSERT(f = fmemopen(buf, strlen(buf), "r"));
	ASSERT(config = pu_config_new());
	ASSERT(reader = pu_config_reader_finit(config, f));

	while(pu_config_reader_next(reader) != -1);

	tap_plan(40);

	tap_ok(reader->eof, "eof reached");
	tap_ok(!reader->error, "no error");
	tap_ok(pu_config_resolve(config) == 0, "finalize config");

	tap_is_str(config->rootdir, "/root2", "RootDir");
	tap_is_str(config->dbpath, "/dbpath1/", "DBPath");
	tap_is_str(config->gpgdir, "gpgdir", "GPGDir");
	tap_is_str(config->logfile, "/logfile", "LogFile");
	tap_is_str(config->xfercommand, "xcommand2", "XferCommand");

	tap_is_int(config->paralleldownloads, 3, "ParallelDownloads");

	tap_ok(config->usesyslog, "UseSyslog");
	tap_ok(config->color, "Color");
	tap_ok(config->checkspace, "CheckSpace");
	tap_ok(config->verbosepkglists, "VerbosePkgLists");
	tap_ok(config->ilovecandy, "ILoveCandy");

	is_siglevel(config->siglevel, 0, "SigLevel");

	i = config->architectures;
	is_str_list(i, "i686", "Arch i686");
	is_str_list(i, "x86_64", "Arch x86_64");
	is_list_exhausted(i, "architectures");

	i = config->ignorepkgs;
	is_str_list(i, "ignorepkga", "IgnorePkg a");
	is_str_list(i, "ignorepkgb", "IgnorePkg b");
	is_list_exhausted(i, "ignorepkg");

	i = config->ignoregroups;
	is_str_list(i, "ignoregroupa", "IgnoreGroup a");
	is_str_list(i, "ignoregroupb", "IgnoreGroup b");
	is_list_exhausted(i, "ignoregroup");

	i = config->noupgrade;
	is_str_list(i, "/tmp/noupgrade*", "NoUpgrade");
	is_list_exhausted(i, "noupgrade");

	i = config->noextract;
	is_str_list(i, "/tmp/noextract*", "NoExtract");
	is_list_exhausted(i, "noextract");

	i = config->cachedirs;
	is_str_list(i, "/cachedir", "CacheDir");
	is_list_exhausted(i, "cachedir");

	i = config->hookdirs;
	is_str_list(i, "/hookdir1", "HookDir");
	is_str_list(i, "/hookdir2", "HookDir");
	is_list_exhausted(i, "hookdir");

	i = config->holdpkgs;
	is_str_list(i, "holdpkga", "HoldPkg a");
	is_str_list(i, "holdpkgb", "HoldPkg b");
	is_list_exhausted(i, "holdpkgs");

	tap_ok(config->repos != NULL, "repo list");

	repo = config->repos->data;
	tap_ok(repo != NULL, "core");
	tap_is_str(repo->name, "core", "repo->name == 'core'");
	tap_is_str(repo->servers->data, "core:i686", "[core] server");

	return tap_finish();
}
