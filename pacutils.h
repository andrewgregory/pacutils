#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>

#include <alpm.h>

#ifndef PACUTILS_H
#define PACUTILS_H

char *pu_version(void);
void pu_print_version(const char *progname, const char *progver);

int pu_pathcmp(const char *p1, const char *p2);
alpm_file_t *pu_filelist_contains_path(alpm_filelist_t *files, const char *path);

/* sync repos */
typedef struct pu_repo_t {
	char *name;
	alpm_siglevel_t siglevel;
	alpm_list_t *servers;
} pu_repo_t;

struct pu_repo_t *pu_repo_new(void);
void pu_repo_free(pu_repo_t *repo);
alpm_db_t *pu_register_syncdb(alpm_handle_t *handle, struct pu_repo_t *repo);
alpm_list_t *pu_register_syncdbs(alpm_handle_t *handle, alpm_list_t *repos);

/* config */
typedef struct pu_config_t {
	char *rootdir;
	char *dbpath;
	char *gpgdir;
	char *logfile;
	char *architecture;
	char *xfercommand;

	unsigned short cleanmethod;
	unsigned short usesyslog;
	unsigned short totaldownload;
	unsigned short checkspace;
	unsigned short verbosepkglists;
	unsigned short color;

	float usedelta;

	alpm_siglevel_t siglevel;
	alpm_siglevel_t localfilesiglevel;
	alpm_siglevel_t remotefilesiglevel;

	alpm_list_t *holdpkgs;
	alpm_list_t *ignorepkgs;
	alpm_list_t *ignoregroups;
	alpm_list_t *noupgrade;
	alpm_list_t *noextract;
	alpm_list_t *cachedirs;

	alpm_list_t *repos;
} pu_config_t;

struct pu_config_t *pu_config_new(void);
struct pu_config_t *pu_config_new_from_file(const char *filename);
void pu_config_free(pu_config_t *config);

alpm_handle_t *pu_initialize_handle_from_config(struct pu_config_t *config);

void pu_cb_download(const char *filename, off_t xfered, off_t total);
const char *pu_msg_progress(alpm_progress_t event);
void pu_cb_progress(alpm_progress_t event, const char *pkgname, int percent,
		size_t total, size_t current);

#endif /* PACUTILS_H */

/* vim: set ts=2 sw=2 noet: */
