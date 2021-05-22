#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "pacutils_test.h"

#include "pacutils.h"

char *tmpdir = NULL, template[] = "/tmp/20-config-includes.c-XXXXXX";
int tmpfd = -1;
char *conf_path = NULL, *include_path = NULL;
pu_config_t *config = NULL;
pu_config_reader_t *reader = NULL;

char pacman_conf[] =
    "\n"
    "[options]\n"
    "Include = %s\n"
    "Server = included_server2\n"
    "[core]\n"
    "Server = core_server\n";

char include_conf[] =
    "XferCommand = included_xfercommand\n"
    "DBPath = included_dbpath\n"
    "[included repo]\n"
    "Server = included_server1\n";

void cleanup(void) {
  pu_config_free(config);
  pu_config_reader_free(reader);
  free(conf_path);
  free(include_path);

  if (tmpfd != -1) { close(tmpfd); }
  if (tmpdir) { rmrfat(AT_FDCWD, tmpdir); }
}

int main(void) {
  pu_repo_t *repo;

  ASSERT(atexit(cleanup) == 0);
  ASSERT(tmpdir = mkdtemp(template));
  ASSERT((tmpfd = open(tmpdir, O_DIRECTORY)) != -1);
  ASSERT(conf_path = pu_asprintf("%s/%s", tmpdir, "pacman.conf"));
  ASSERT(include_path = pu_asprintf("%s/%s", tmpdir, "include.conf"));
  ASSERT(spew(tmpfd, "pacman.conf", pacman_conf, include_path) == 0);
  ASSERT(spew(tmpfd, "include.conf", include_conf) == 0);

  ASSERT(config = pu_config_new());
  ASSERT(reader = pu_config_reader_new(config, conf_path));

  while (pu_config_reader_next(reader) != -1);

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
