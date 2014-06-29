#include <string.h>
#include <stdlib.h>

#include "pacutils.h"

#include "tap.h"

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

int main(int argc, char **argv) {
    alpm_list_t *i;
    pu_config_t *config;
    pu_repo_t *repo;
    char temp_path1[] = "pu_config_test_XXXXXX";
    char temp_path2[] = "pu_config_test_XXXXXX";
    int fd1 = mkstemp(temp_path1);
    int fd2 = mkstemp(temp_path2);
    FILE *stream1 = fdopen(fd1, "r+");
    FILE *stream2 = fdopen(fd2, "r+");
    fprintf(stream2,
            "XferCommand = included_xfercommand\n"
            "[included repo]\n"
            "SigLevel = PackageOptional\n"
            "");
    fprintf(stream1,
            "\n"
            "[options]\n"
            "#RootDir = /root1\n"
            "RootDir = /root2\n"
            "DBPath = /dbpath1/ \n"
            "DBPath = /dbpath2/ \n"
            "CacheDir=/cachedir\n"
            "GPGDir =gpgdir\n"
            "LogFile= /logfile #ignore me\n"
            " HoldPkg = holdpkga holdpkgb \n"
            "IgnorePkg = ignorepkga\n"
            "IgnorePkg = ignorepkgb\n"
            " IgnoreGroup = ignoregroupa ignoregroupb \n"
            "Architecture = i686\n"
            "NoUpgrade = /tmp/noupgrade*\n"
            "NoExtract = /tmp/noextract*\n"
            "CleanMethod = KeepInstalled KeepCurrent\n"
            "UseSyslog\n"
            "Color\n"
            "UseDelta\n"
            "TotalDownload\n"
            "CheckSpace\n"
            "VerbosePkgLists\n"
            "ILoveCandy\n"
            "SigLevel = Never\n"
            "Include = %s\n"
            "SigLevel = PackageRequired\n"

            "[core]\n"
            "Server = $repo:$arch\n"
            "",
            temp_path2);

    fclose(stream1);
    fclose(stream2);

	tap_plan(37);

    config = pu_config_new_from_file(temp_path1);

    tap_ok(config != NULL, "config != NULL");
    if(!config) {
        tap_bail("pu_config_new_from_file failed");
    }

    tap_is_str(config->rootdir, "/root2", "RootDir");

    tap_todo("review pacman parser before \"fixing\"");
    tap_is_str(config->dbpath, "/dbpath1/", "DBPath");
    tap_todo(NULL);

    tap_is_str(config->gpgdir, "gpgdir", "DPGDir");
    tap_is_str(config->logfile, "/logfile", "LogFile");
    tap_is_str(config->architecture, "i686", "Arch");

    tap_ok(config->usesyslog, "UseSyslog");
    tap_ok(config->color, "Color");
    tap_ok(config->totaldownload, "TotalDownload");
    tap_ok(config->checkspace, "CheckSpace");
    tap_ok(config->verbosepkglists, "VerbosePkgLists");
    tap_ok(config->ilovecandy, "ILoveCandy");

    tap_is_int(config->siglevel, 0, "SigLevel");

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

    i = config->holdpkgs;
    is_str_list(i, "holdpkga", "HoldPkg a");
    is_str_list(i, "holdpkgb", "HoldPkg b");
    is_list_exhausted(i, "holdpkgs");
#undef tap_is_str_list

    tap_is_float(config->usedelta, 0.7, 0.0001, "UseDelta (default)");

    tap_is_str(config->xfercommand, "included_xfercommand", "Include XferCommand");

    tap_ok(config->repos != NULL, "repo list");

    repo = config->repos->data;
    tap_ok(repo != NULL, "included repo");
    tap_is_str(repo->name, "included repo", "repo->name == 'included repo'");
    tap_is_int(repo->siglevel, ALPM_SIG_PACKAGE, "[included repo] SigLevel");

    repo = config->repos->next->data;
    tap_ok(repo != NULL, "core");
    tap_is_str(repo->name, "core", "repo->name == 'core'");
    tap_is_str(repo->servers->data, "core:i686", "[core] server");

    unlink(temp_path1);
    unlink(temp_path2);

	return tap_tests_failed();
}
