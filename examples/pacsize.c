#include <getopt.h>

#include <pacutils.h>

/*********************************************
 * Displays the size of an installed package
 *********************************************/

/**
 * @brief Calculate the size of a package and its removable dependencies.
 *
 * @param pkg package to calculate
 * @param pkgs list of installed packages
 * @param seen packages already counted in the removable size
 *
 * @return removable size in bytes
 */
off_t pkg_size(alpm_pkg_t *pkg, alpm_list_t *pkgs, alpm_list_t **seen)
{
	const char *pkgname = alpm_pkg_get_name(pkg);
	if(alpm_list_find_ptr(*seen, pkgname))
		return 0;


	off_t size = alpm_pkg_get_isize(pkg);
	*seen = alpm_list_add(*seen,(void*) pkgname);

	alpm_list_t *d, *deps = alpm_pkg_get_depends(pkg);
	for(d = deps; d; d = d->next) {
		char *depstring = alpm_dep_compute_string(d->data);
		alpm_pkg_t *p = alpm_find_satisfier(pkgs, depstring);
		free(depstring);

		if(!p) {
			continue;
		}

		if(alpm_pkg_get_reason(p) == ALPM_PKG_REASON_EXPLICIT) {
			continue;
		}

		if(alpm_list_find_ptr(*seen, p)) {
			continue;
		}

		alpm_list_t *rb = alpm_pkg_compute_requiredby(p);

		if(rb) {
			alpm_list_t *diff = alpm_list_diff(rb, *seen,
					(alpm_list_fn_cmp) strcmp);
			if(diff) {
				alpm_list_free(diff);
				FREELIST(rb);
				continue;
			}
			FREELIST(rb);
		}

		size += pkg_size(p, pkgs, seen);
	}

	return size;
}

char *hr_size(off_t bytes) {
	int magnitude = 0;
	float b = (float) bytes;
	char *size = malloc(15);
	const char prefixes[] = {'B', 'K', 'M', 'G', 'T'};

	while(b > 1000) {
		b /= 1024;
		magnitude++;
	}

	snprintf(size, 14, "%.3f %c", b, prefixes[magnitude]);
	return size;
}

void usage(int ret) {
	FILE *stream = (ret ? stderr : stdout);
	fputs("pacsize - calculate removable size of packages\n", stream);
	fputs("usage:  pacsize [options] <package>...\n", stream);
	fputs("        pacsize (--help|--version)\n", stream);
	fputs("options:\n", stream);
	fputs("   --config=<path>    set an alternate configuration file\n", stream);
	fputs("   --help             display this help information\n", stream);
	fputs("   --version          display version information\n", stream);
	exit(ret);
}

void version(void) {
	printf("pacsize v0.1 - libalpm %s\n", alpm_version());
	exit(0);
}

pu_config_t *parse_opts(int argc, char **argv) {
	char *config_file = "/etc/pacman.conf";

	struct option long_opts[] = {
		{"help"    , no_argument       , NULL , 'h'} ,
		{"version" , no_argument       , NULL , 'V'} ,
		{"config"  , required_argument , NULL , 'c'} ,
		{0         , 0                 , 0    , 0}   ,
	};

	int c = getopt_long(argc, argv, "", long_opts, NULL);
	while(c != -1) {
		switch(c) {
			case 'h':
				usage(0);
				break;
			case 'V':
				version();
				break;
			case 'c':
				config_file = optarg;
				break;
			case '?':
			default:
				usage(1);
				break;
		}

		c = getopt_long(argc, argv, "", long_opts, NULL);
	}

	return pu_config_new_from_file(config_file);
}

int main(int argc, char **argv) {
	struct pu_config_t *config = parse_opts(argc, argv);
	alpm_handle_t *handle = pu_initialize_handle_from_config(config);
	alpm_db_t *localdb = alpm_get_localdb(handle);
	alpm_list_t *seen = NULL, *pkgs = alpm_db_get_pkgcache(localdb);
	off_t size = 0;

	for(; *argv; ++argv) {
		alpm_pkg_t *p = alpm_find_satisfier(pkgs, *argv);
		if(p) {
			size += pkg_size(p, pkgs, &seen);
		}
	}

	char *s = hr_size(size);
	puts(s);
	free(s);

	/* cleanup */
	alpm_list_free(seen);
	alpm_release(handle);
	pu_config_free(config);

	return 0;
}

/* vim: set ts=2 sw=2 noet: */
