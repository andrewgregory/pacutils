#include <stdio.h>

#include <pacutils.h>

/*********************************************
 * Displays installed package info ala pacman -Qi
 *********************************************/

void printt(const char *field, alpm_time_t t) {
		char time_buf[26];
		struct tm ltime;
		fputs(field, stdout);
		if(t) {
			strftime(time_buf, 26, "%F %T", localtime_r(&t, &ltime));
			fputs(time_buf, stdout);
		}
		putchar('\n');
}

void printl(const char *field, alpm_list_t *values) {
	fputs(field, stdout);
	while(values) {
		fputs(values->data, stdout);
		values = values->next;
		if(values) {
			putchar(' ');
		}
	}
	putchar('\n');
}

void printd(const char *field, alpm_list_t *values) {
	fputs(field, stdout);
	while(values) {
		char *s = alpm_dep_compute_string(values->data);
		fputs(s, stdout);
		free(s);
		values = values->next;
		if(values) {
			putchar(' ');
		}
	}
	putchar('\n');
}

int main(int argc, char **argv) {
	struct pu_config_t *config = pu_config_new_from_file("/etc/pacman.conf");
	alpm_handle_t *handle = pu_initialize_handle_from_config(config);
	alpm_list_t *syncdbs = pu_register_syncdbs(handle, config->repos);

	for(++argv; *argv; ++argv) {
		alpm_pkg_t *pkg = pu_find_pkgspec(handle, *argv);
		if(!pkg) {
			fprintf(stderr, "Unable to load package '%s'\n", *argv);
			continue;
		}

		printf("Name:           %s\n",  alpm_pkg_get_name(pkg));
		printf("Repository:     %s\n",  alpm_db_get_name(alpm_pkg_get_db(pkg)));
		printf("File:           %s\n",  alpm_pkg_get_filename(pkg));
		printf("Version:        %s\n",  alpm_pkg_get_version(pkg));
		printf("Description:    %s\n",  alpm_pkg_get_desc(pkg));
		printf("Architecture:   %s\n",  alpm_pkg_get_arch(pkg));
		printf("URL:            %s\n",  alpm_pkg_get_url(pkg));
		printl("Licenses:       ",      alpm_pkg_get_licenses(pkg));
		printl("Groups:         ",      alpm_pkg_get_groups(pkg));
		printd("Provides:       ",      alpm_pkg_get_provides(pkg));
		printd("Requires:       ",      alpm_pkg_get_depends(pkg));
		printd("Conflicts:      ",      alpm_pkg_get_conflicts(pkg));
		printd("Replaces:       ",      alpm_pkg_get_replaces(pkg));
		printf("Download Size:  %zd\n", alpm_pkg_get_size(pkg));
		printf("Installed Size: %zd\n", alpm_pkg_get_isize(pkg));
		printf("Packager:       %s\n",  alpm_pkg_get_packager(pkg));
		printt("Build Date:     ", alpm_pkg_get_builddate(pkg));
		printt("Install Date:   ", alpm_pkg_get_installdate(pkg));

		/*printd("Required For:   ",      alpm_pkg_get_provides(pkg));*/
		/*printd("Optional For:   ",      alpm_pkg_get_provides(pkg));*/
		/* install reason */
		/* install script */
		/* validated by */

		if(*(argv+1)) {
			putchar('\n');
		}
	}

	/* cleanup */
	alpm_list_free(syncdbs);
	alpm_release(handle);
	pu_config_free(config);

	return 0;
}

/* vim: set ts=2 sw=2 noet: */
