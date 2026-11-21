#include "test.h"

/* Include the implementation to reach the file-local guard_name helper. */
#include "header.c"

int main(void) {
  /* Error paths must not crash or return a literal that callers can free. */
  CHECK(header_path(NULL, 0) == NULL);
  CHECK(header_path("foo.c", 0) == NULL);
  CHECK(header_path("foo.c", 1) == NULL);
  CHECK(header_path("foo.c", 2) == NULL);
  CHECK(header_path("foo.h", 5) == NULL);
  CHECK(header_path("foo", 3) == NULL);

  /* Valid paths produce a heap-allocated .h path. */
  {
    char *h = header_path("foo.c", 5);
    CHECK(h != NULL);
    if (h != NULL) {
      CHECK_STR(h, "foo.h");
      free(h);
    }
  }
  {
    const char *path = "dir/sub/name.c";
    char *h = header_path(path, strlen(path));
    CHECK(h != NULL);
    if (h != NULL) {
      CHECK_STR(h, "dir/sub/name.h");
      free(h);
    }
  }
  {
    const char *path = "x.c";
    char *h = header_path(path, strlen(path));
    CHECK(h != NULL);
    if (h != NULL) {
      CHECK_STR(h, "x.h");
      free(h);
    }
  }

  /* guard_name: basename only, valid identifier, deterministic. */
  CHECK(guard_name(NULL) == NULL);
  {
    char *g = guard_name("dir/kmp.h");
    CHECK(g != NULL);
    if (g != NULL) {
      CHECK_STR(g, "HGEN_KMP_H");
      free(g);
    }
  }
  {
    char *g = guard_name("weird-name.1.h");
    CHECK(g != NULL);
    if (g != NULL) {
      CHECK_STR(g, "HGEN_WEIRD_NAME_1_H");
      free(g);
    }
  }
  {
    char *g = guard_name("dir/");
    CHECK(g != NULL);
    if (g != NULL) {
      CHECK_STR(g, "HGEN_");
      free(g);
    }
  }

  TEST_REPORT("header");
}
