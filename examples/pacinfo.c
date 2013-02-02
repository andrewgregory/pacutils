#include <stdio.h>

#include <pacutils.h>

/*********************************************
 * Displays installed package info ala pacman -Qi
 *********************************************/

int main(int argc, char **argv) {
    struct pu_config_t *config = pu_config_new_from_file("/etc/pacman.conf");
    alpm_handle_t *handle = pu_initialize_handle_from_config(config);
    alpm_db_t *localdb = alpm_get_localdb(handle);
    alpm_list_t *pkgs = alpm_db_get_pkgcache(localdb);

    for(; *argv; ++argv) {
        alpm_pkg_t *p = alpm_find_satisfier(pkgs, *argv);
        if(p) {
            printf("%s %s\n", alpm_pkg_get_name(p), alpm_pkg_get_version(p));
        }
    }

    /* cleanup */
    alpm_release(handle);
		pu_config_free(config);

    return 0;
}

/* vim: set ts=2 sw=2 noet: */
