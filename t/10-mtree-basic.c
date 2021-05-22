#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "pacutils.h"

#include "pacutils_test.h"

FILE *stream = NULL;
pu_mtree_reader_t *reader = NULL;

void cleanup(void) {
  pu_mtree_reader_free(reader);
  fclose(stream);
}

char buf[] =
    "#mtree\n"
    "/set type=file uid=0 gid=0 mode=644\n"
    "./.BUILDINFO time=1453283269.954514837 size=25444 md5digest=1b30bf27a1f20eef4402ecc95134844a sha256digest=e21cb92b5239f423ce578a45575b425ea0583aa9245f501c8e54d19b8bfe16b1\n"
    "./.PKGINFO time=1453283269.864514835 size=410 md5digest=b90ee962592f6c66c2ccbfa3718ebdce sha256digest=863dfa105c333a4e91d70b7332931e99fc5561a35685039e098e4b261466eae0\n"
    "/set mode=755\n"
    "./usr time=1453283269.234514817 type=dir\n"
    "./usr/bin time=1453283269.447848157 type=dir\n"
    "./usr/bin/paccheck time=1453283269.447848157 size=23712 md5digest=adeb5af3c33e76f0e663394c88272c14 sha256digest=0669f596e333e053f61ee9a2c6b443a9a3ef2c2640fe6bf67acc502d67d9b51b\n"
    "";

int main(void) {
  pu_mtree_t *e;

  ASSERT(stream = fmemopen(buf, strlen(buf), "r"));
  ASSERT(reader = pu_mtree_reader_open_stream(stream));

  tap_plan(47);

  tap_ok((e = pu_mtree_reader_next(reader, NULL)) != NULL, "next");
  tap_is_str(e->path, ".BUILDINFO", "path");
  tap_is_str(e->type, "file", "type");
  tap_is_int(e->uid, 0, "uid");
  tap_is_int(e->gid, 0, "gid");
  tap_is_int(e->mode, 0644, "mode");
  tap_is_int(e->size, 25444, "time");
  tap_is_str(e->md5digest, "1b30bf27a1f20eef4402ecc95134844a", "md5");
  tap_ok(!reader->eof, "eof");
  pu_mtree_free(e);

  tap_ok((e = pu_mtree_reader_next(reader, NULL)) != NULL, "next");
  tap_is_str(e->path, ".PKGINFO", "path");
  tap_is_str(e->type, "file", "type");
  tap_is_int(e->uid, 0, "uid");
  tap_is_int(e->gid, 0, "gid");
  tap_is_int(e->mode, 0644, "mode");
  tap_is_int(e->size, 410, "time");
  tap_is_str(e->md5digest, "b90ee962592f6c66c2ccbfa3718ebdce", "md5");
  tap_ok(!reader->eof, "eof");
  pu_mtree_free(e);

  tap_ok((e = pu_mtree_reader_next(reader, NULL)) != NULL, "next");
  tap_is_str(e->path, "usr", "path");
  tap_is_str(e->type, "dir", "type");
  tap_is_int(e->uid, 0, "uid");
  tap_is_int(e->gid, 0, "gid");
  tap_is_int(e->mode, 0755, "mode");
  tap_is_int(e->size, 0, "time");
  tap_is_str(e->md5digest, "", "md5");
  tap_ok(!reader->eof, "eof");
  pu_mtree_free(e);

  tap_ok((e = pu_mtree_reader_next(reader, NULL)) != NULL, "next");
  tap_is_str(e->path, "usr/bin", "path");
  tap_is_str(e->type, "dir", "type");
  tap_is_int(e->uid, 0, "uid");
  tap_is_int(e->gid, 0, "gid");
  tap_is_int(e->mode, 0755, "mode");
  tap_is_int(e->size, 0, "time");
  tap_is_str(e->md5digest, "", "md5");
  tap_ok(!reader->eof, "eof");
  pu_mtree_free(e);

  tap_ok((e = pu_mtree_reader_next(reader, NULL)) != NULL, "next");
  tap_is_str(e->path, "usr/bin/paccheck", "path");
  tap_is_str(e->type, "file", "type");
  tap_is_int(e->uid, 0, "uid");
  tap_is_int(e->gid, 0, "gid");
  tap_is_int(e->mode, 0755, "mode");
  tap_is_int(e->size, 23712, "time");
  tap_is_str(e->md5digest, "adeb5af3c33e76f0e663394c88272c14", "md5");
  tap_ok(!reader->eof, "eof");
  pu_mtree_free(e);

  tap_ok(pu_mtree_reader_next(reader, NULL) == NULL, "next");
  tap_ok(reader->eof, "eof");

  return tap_finish();
}
