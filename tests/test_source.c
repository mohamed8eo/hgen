#include "test.h"

#include "fileio.h"
#include "scan.h"
#include "source.h"

int main(void) {
  /* --- source_has_own_include ----------------------------------------- */
  CHECK(source_has_own_include(NULL, "buf.h") == -1);
  CHECK(source_has_own_include("int x;\n", NULL) == -1);
  CHECK(source_has_own_include("#include \"buf.h\"\n", "dir/buf.h") == 1);
  CHECK(source_has_own_include("#include <buf.h>\n", "buf.h") == 1);
  CHECK(source_has_own_include("# include   \"buf.h\"\n", "buf.h") == 1);
  CHECK(source_has_own_include("  #include \"buf.h\"\n", "buf.h") == 1);
  CHECK(source_has_own_include("/* #include \"buf.h\" */\n", "buf.h") == 0);
  CHECK(source_has_own_include("#include \"other.h\"\n", "buf.h") == 0);
  CHECK(source_has_own_include("int x;\n", "buf.h") == 0);

  /* --- source_rewrite: remove struct, add include, write once --------- */
  {
    const char *c_path = "build/test_source.c";
    const char *h_path = "build/test_source.h";
    const char *src = "typedef struct {\n"
                      "  int x;\n"
                      "} Pub;\n"
                      "Pub *make(void) { return 0; }\n";
    const char *want = "#include \"test_source.h\"\n"
                       "Pub *make(void) { return 0; }\n";
    FILE *fp = fopen(c_path, "w");
    CHECK(fp != NULL);
    if (fp != NULL) {
      fputs(src, fp);
      fclose(fp);
    }

    ItemList items = {0};
    scan(src, &items);

    int added = -1;
    CHECK(source_rewrite(src, &items, h_path, c_path, &added) == 0);
    CHECK_INT(added, 1);

    size_t len = 0;
    char *got = read_file(c_path, &len);
    CHECK(got != NULL);
    if (got != NULL) {
      CHECK_STR(got, want);
      free(got);
    }

    /* The include is already there now, so the check must skip it. */
    added = -1;
    CHECK(source_ensure_include(c_path, h_path, &added) == 0);
    CHECK_INT(added, 0);
    got = read_file(c_path, &len);
    CHECK(got != NULL);
    if (got != NULL) {
      CHECK_STR(got, want);
      free(got);
    }

    remove(c_path);
    itemlist_free(&items);
  }

  /* --- source_rewrite: existing include is left alone ----------------- */
  {
    const char *c_path = "build/test_source2.c";
    const char *h_path = "build/test_source2.h";
    const char *src = "#include \"test_source2.h\"\n"
                      "typedef struct {\n"
                      "  int x;\n"
                      "} Pub;\n"
                      "Pub *make(void) { return 0; }\n";
    const char *want = "#include \"test_source2.h\"\n"
                       "Pub *make(void) { return 0; }\n";
    FILE *fp = fopen(c_path, "w");
    CHECK(fp != NULL);
    if (fp != NULL) {
      fputs(src, fp);
      fclose(fp);
    }

    ItemList items = {0};
    scan(src, &items);

    int added = -1;
    CHECK(source_rewrite(src, &items, h_path, c_path, &added) == 0);
    CHECK_INT(added, 0);

    size_t len = 0;
    char *got = read_file(c_path, &len);
    CHECK(got != NULL);
    if (got != NULL) {
      CHECK_STR(got, want);
      free(got);
    }

    remove(c_path);
    itemlist_free(&items);
  }

  TEST_REPORT("source");
}
