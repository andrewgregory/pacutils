#include <stdio.h>

#include <pacutils.h>

/*********************************************
 * Displays installed package info ala pacman -Qi
 *********************************************/

int main(int argc, char **argv) {
	struct pu_config_t *config = pu_config_new_from_file("/etc/pacman.conf");
	alpm_handle_t *handle = pu_initialize_handle_from_config(config);

	for(++argv; *argv; ++argv) {
		alpm_pkg_t *pkg = pu_find_pkgspec(handle, *argv);
		if(!pkg) {
			fprintf(stderr, "Unable to load package '%s'\n", *argv);
			continue;
		}

		printf("Name:         %s\n", alpm_pkg_get_name(pkg));
		printf("Version:      %s\n", alpm_pkg_get_version(pkg));
		printf("Description:  %s\n", alpm_pkg_get_desc(pkg));
		printf("Architecture: %s\n", alpm_pkg_get_arch(pkg));
		printf("URL:          %s\n", alpm_pkg_get_url(pkg));
		printf("Packager:     %s\n", alpm_pkg_get_packager(pkg));

		if(*(argv+1)) {
			putchar('\n');
		}
	}

	/* cleanup */
	alpm_release(handle);
	pu_config_free(config);

	return 0;
}

/* vim: set ts=2 sw=2 noet: */
