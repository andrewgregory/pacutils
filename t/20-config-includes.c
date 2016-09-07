#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "pacutils.h"

#include "tap.h"

char mockfile1[] = 
    "\n"
    "[options]\n"
    "Include = mockfile2.ini\n"
    "Server = included_server2\n"
    "[core]\n"
    "Server = core_server\n";
char mockfile2[] = 
    "XferCommand = included_xfercommand\n"
    "DBPath = included_dbpath\n"
    "[included repo]\n"
    "Server = included_server1\n";

FILE *fopen(const char *path, const char *mode) {
    if(strcmp(path, "mockfile1.ini") == 0) {
        return fmemopen(mockfile1, strlen(mockfile1), mode);
    } else if(strcmp(path, "mockfile2.ini") == 0) {
        return fmemopen(mockfile2, strlen(mockfile2), mode);
    } else  {
        tap_diag("attempted to open non-mocked file '%s'", path);
        return NULL;
    }
}

int main(void) {
    pu_config_t *config = pu_config_new();
    pu_config_reader_t *reader = pu_config_reader_new(config, "mockfile1.ini");
    pu_repo_t *repo;

    if(config == NULL || reader == NULL) {
        tap_bail("error initializing reader (%s)", strerror(errno));
        return 1;
    }
    while(pu_config_reader_next(reader) != -1);

    tap_plan(13);

    tap_ok(reader->eof, "eof reached");
    tap_ok(!reader->error, "no error");
    tap_ok(pu_config_resolve(config) == 0, "finalize config");

    tap_is_str(config->xfercommand, "included_xfercommand", "Include XferCommand");
    tap_is_str(config->dbpath, "included_dbpath", "Include DBPath");

    tap_ok(config->repos != NULL, "repo list");

    repo = config->repos->data;
    tap_ok(repo != NULL, "included repo");
    tap_is_str(repo->name, "included repo", "repo->name == 'included repo'");
    tap_is_str(repo->servers->data, "included_server1", "[included] server1");
    tap_is_str(repo->servers->next->data, "included_server2", "[included] server2");

    repo = config->repos->next->data;
    tap_ok(repo != NULL, "core");
    tap_is_str(repo->name, "core", "repo->name == 'core'");
    tap_is_str(repo->servers->data, "core_server", "[core] server");

	return 0;
}
