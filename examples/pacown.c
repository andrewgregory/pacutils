#include <stdio.h>

#include <pacutils.h>

int main(int argc, char **argv) {
    pu_config_t *config = pu_config_new_from_file("/etc/pacman.conf");
    alpm_handle_t *handle = pu_initialize_handle_from_config(config);
    alpm_db_t *localdb = alpm_get_localdb(handle);
    alpm_list_t *t, *pkgs = alpm_db_get_pkgcache(localdb);
    int ret = 0;

    /* read targets from the command line */
    for(argv++; *argv; argv++) {
        const char *file = *argv;
        alpm_list_t *p;
        int found = 0;

        for(p = pkgs; p; p = p->next) {
            alpm_pkg_t *pkg = p->data;
            if(alpm_filelist_contains(alpm_pkg_get_files(pkg), file)) {
                printf("%s is owned by %s %s\n", file,
                        alpm_pkg_get_name(pkg), alpm_pkg_get_version(pkg));
                found = 1;
            }
        }

        if(!found) {
            printf("%s is unowned\n", file);
            ret = 1;
        }
    }

    alpm_release(handle);
    pu_config_free(config);

    return ret;
}
