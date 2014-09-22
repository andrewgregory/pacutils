#include <string.h>
#include <stdlib.h>

#include "pacutils.h"

#include "tap.h"

char mockfile1[] = 
    "\n"
    "[options]\n"
    "Include = mockfile2.ini\n"
    "Server = included_server\n"
    "[core]\n"
    "Server = core_server\n";
char mockfile2[] = 
    "XferCommand = included_xfercommand\n"
    "[included repo]\n";

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
    pu_config_t *config;
    pu_repo_t *repo;

	tap_plan(9);

    config = pu_config_new_from_file("mockfile1.ini");
    if(!tap_ok(config != NULL, "config != NULL")) {
        tap_bail("pu_config_new_from_file failed");
        return 1;
    }

    tap_is_str(config->xfercommand, "included_xfercommand", "Include XferCommand");

    tap_ok(config->repos != NULL, "repo list");

    repo = config->repos->data;
    tap_ok(repo != NULL, "included repo");
    tap_is_str(repo->name, "included repo", "repo->name == 'included repo'");
    tap_is_str(repo->servers->data, "included_server", "[core] server");

    repo = config->repos->next->data;
    tap_ok(repo != NULL, "core");
    tap_is_str(repo->name, "core", "repo->name == 'core'");
    tap_is_str(repo->servers->data, "core_server", "[core] server");

	return 0;
}
