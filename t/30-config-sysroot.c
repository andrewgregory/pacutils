#include <stdlib.h>
#include <fcntl.h>

#include "pacutils_test.h"

#include "pacutils.h"

char *tmpdir = NULL, template[] = "/tmp/30-config-sysroot.c-XXXXXX";
int tmpfd = -1;
char *rootdir = NULL, *dbpath = NULL, *server = NULL;
pu_repo_t *r = NULL;
pu_config_t *config = NULL;
pu_config_reader_t *reader = NULL;

char pacman_conf[] =
"[options]\n"
"RootDir = 30-config-sysroot-RootDir\n"
"Include = /etc/include.conf\n";

char include_conf[] =
"DBPath = 30-config-sysroot-DBPath\n"
"[repo]\n"
"Server = http://somehost/path\n"
"Server = file:///30-config-sysroot-Server/\n";

void cleanup(void) {
	pu_config_free(config);
	pu_config_reader_free(reader);

	free(rootdir);
	free(dbpath);

	if(tmpfd != -1) { close(tmpfd); }
	if(tmpdir) { rmrfat(AT_FDCWD, tmpdir); }
}

int main(void) {
	ASSERT(atexit(cleanup) == 0);
	ASSERT(tmpdir = mkdtemp(template));
	ASSERT((tmpfd = open(tmpdir, O_DIRECTORY)) != -1);
	ASSERT(mkdirat(tmpfd, "etc", 0777) == 0);
	ASSERT(spew(tmpfd, "etc/pacman.conf", pacman_conf) == 0);
	ASSERT(spew(tmpfd, "etc/include.conf", include_conf) == 0);
	ASSERT(rootdir = pu_asprintf("%s/%s", template, "30-config-sysroot-RootDir"));
	ASSERT(dbpath = pu_asprintf("%s/%s", template, "30-config-sysroot-DBPath"));
	ASSERT(server = pu_asprintf("file://%s/%s", template, "30-config-sysroot-Server/"));

	ASSERT(config = pu_config_new());
	ASSERT(reader = pu_config_reader_new_sysroot(config, "/etc/pacman.conf", tmpdir));

	while(pu_config_reader_next(reader) != -1);
	r = config->repos->data;

	tap_plan(12);

	tap_is_int(reader->status, PU_CONFIG_READER_STATUS_OK, "reader status");
	tap_ok(reader->eof, "reader eof");
	tap_ok(!reader->error, "reader error");

	/* raw values */
	tap_is_str(config->rootdir, "30-config-sysroot-RootDir", "rootdir");
	tap_is_str(config->dbpath, "30-config-sysroot-DBPath", "dbpath");
	tap_is_str(r->servers->data, "http://somehost/path", "http:// server");
	tap_is_str(r->servers->next->data, "file:///30-config-sysroot-Server/", "file:// server");

	/* resolved values */
	tap_is_int(pu_config_resolve_sysroot(config, tmpdir), 0, "resolve_sysroot");
	tap_is_str(config->rootdir, rootdir, "resolved rootdir");
	tap_is_str(config->dbpath, dbpath, "resolved dbpath");

	tap_is_str(r->servers->data, "http://somehost/path", "resolved http:// server");
	tap_is_str(r->servers->next->data, server, "resolved file:// server");

	return tap_finish();
}
