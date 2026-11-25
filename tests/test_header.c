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
    char *g = guard_name("dir/1counter.h");
    CHECK(g != NULL);
    if (g != NULL) {
      CHECK_STR(g, "HGEN_1COUNTER_H");
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

  /* --- header_build: public structs are emitted, private ones are not ---- */
  {
    const char *src = "typedef struct {\n"
                      "  int x;\n"
                      "} Pub;\n"
                      "Pub *make_pub(void) {\n"
                      "  return 0;\n"
                      "}\n";
    const char *want = "#ifndef HGEN_FOO_H\n"
                       "#define HGEN_FOO_H\n"
                       "\n"
                       "typedef struct {\n"
                       "  int x;\n"
                       "} Pub;\n"
                       "\n"
                       "/* --- BEGIN PROTOTYPES (auto-generated, do not edit) --- */\n"
                       "Pub *make_pub(void);\n"
                       "/* --- END PROTOTYPES --- */\n"
                       "\n"
                       "#endif /* HGEN_FOO_H */\n";
    ItemList items = {0};
    Buf out = {0};
    scan(src, &items);
    CHECK(header_build(src, "foo.h", &items, &out) == 0);
    CHECK(out.data != NULL);
    if (out.data != NULL)
      CHECK_STR(out.data, want);
    itemlist_free(&items);
    buf_free(&out);
  }
  {
    const char *src = "struct Hidden {\n"
                      "  int x;\n"
                      "};\n"
                      "int pub(void) { return 0; }\n";
    const char *want = "#ifndef HGEN_FOO_H\n"
                       "#define HGEN_FOO_H\n"
                       "\n"
                       "/* --- BEGIN PROTOTYPES (auto-generated, do not edit) --- */\n"
                       "int pub(void);\n"
                       "/* --- END PROTOTYPES --- */\n"
                       "\n"
                       "#endif /* HGEN_FOO_H */\n";
    ItemList items = {0};
    Buf out = {0};
    scan(src, &items);
    CHECK(header_build(src, "foo.h", &items, &out) == 0);
    CHECK(out.data != NULL);
    if (out.data != NULL)
      CHECK_STR(out.data, want);
    itemlist_free(&items);
    buf_free(&out);
  }
  {
    const char *src = "typedef struct {\n"
                      "  size_t n;\n"
                      "} B;\n"
                      "B *make(void) { return 0; }\n";
    const char *want = "#ifndef HGEN_FOO_H\n"
                       "#define HGEN_FOO_H\n"
                       "\n"
                       "#include <stddef.h>\n"
                       "\n"
                       "typedef struct {\n"
                       "  size_t n;\n"
                       "} B;\n"
                       "\n"
                       "/* --- BEGIN PROTOTYPES (auto-generated, do not edit) --- */\n"
                       "B *make(void);\n"
                       "/* --- END PROTOTYPES --- */\n"
                       "\n"
                       "#endif /* HGEN_FOO_H */\n";
    ItemList items = {0};
    Buf out = {0};
    scan(src, &items);
    CHECK(header_build(src, "foo.h", &items, &out) == 0);
    CHECK(out.data != NULL);
    if (out.data != NULL)
      CHECK_STR(out.data, want);
    itemlist_free(&items);
    buf_free(&out);
  }
  {
    const char *src = "struct Foo {\n"
                      "  int x;\n"
                      "};\n"
                      "struct Foo *make(void) { return 0; }\n";
    const char *want = "#ifndef HGEN_FOO_H\n"
                       "#define HGEN_FOO_H\n"
                       "\n"
                       "struct Foo {\n"
                       "  int x;\n"
                       "};\n"
                       "\n"
                       "/* --- BEGIN PROTOTYPES (auto-generated, do not edit) --- */\n"
                       "struct Foo *make(void);\n"
                       "/* --- END PROTOTYPES --- */\n"
                       "\n"
                       "#endif /* HGEN_FOO_H */\n";
    ItemList items = {0};
    Buf out = {0};
    scan(src, &items);
    CHECK(header_build(src, "foo.h", &items, &out) == 0);
    CHECK(out.data != NULL);
    if (out.data != NULL)
      CHECK_STR(out.data, want);
    itemlist_free(&items);
    buf_free(&out);
  }

  /* --- header_update: only the generated block changes. Everything above
         BEGIN and below END is kept, and duplicated END markers from older
         runs collapse into a single one. */
  {
    const char *src = "typedef struct {\n"
                      "  int x;\n"
                      "} Pub;\n"
                      "Pub *make_pub(void) { return 0; }\n"
                      "int extra(int a) { return a; }\n";
    const char *old = "#ifndef HGEN_FOO_H\n"
                      "#define HGEN_FOO_H\n"
                      "\n"
                      "typedef struct {\n"
                      "  int x;\n"
                      "} Pub;\n"
                      "\n"
                      "/* --- BEGIN PROTOTYPES (auto-generated, do not edit) --- */\n"
                      "Pub *old_api(void);\n"
                      "/* --- END PROTOTYPES --- */\n"
                      "/* --- END PROTOTYPES --- */\n"
                      "\n"
                      "#endif /* HGEN_FOO_H */\n";
    const char *want = "#ifndef HGEN_FOO_H\n"
                       "#define HGEN_FOO_H\n"
                       "\n"
                       "typedef struct {\n"
                       "  int x;\n"
                       "} Pub;\n"
                       "\n"
                       "/* --- BEGIN PROTOTYPES (auto-generated, do not edit) --- */\n"
                       "Pub *make_pub(void);\n"
                       "int extra(int a);\n"
                       "/* --- END PROTOTYPES --- */\n"
                       "\n"
                       "#endif /* HGEN_FOO_H */\n";
    ItemList items = {0};
    Buf out = {0};
    scan(src, &items);
    CHECK(header_update(old, &items, &out) == 0);
    CHECK(out.data != NULL);
    if (out.data != NULL)
      CHECK_STR(out.data, want);
    itemlist_free(&items);
    buf_free(&out);
  }

  /* Refuse malformed headers without touching the output buffer. */
  CHECK(header_update(NULL, NULL, NULL) == -1);
  {
    ItemList items = {0};
    Buf out = {0};
    const char *old = "#ifndef X\n#define X\n#endif\n";
    CHECK(header_update(old, &items, &out) == -1);
    CHECK(out.data == NULL);
    itemlist_free(&items);
    buf_free(&out);
  }
  {
    ItemList items = {0};
    Buf out = {0};
    const char *old = "/* --- END PROTOTYPES --- */\n"
                      "/* --- BEGIN PROTOTYPES (auto-generated, do not edit) --- */\n";
    CHECK(header_update(old, &items, &out) == -1);
    CHECK(out.data == NULL);
    itemlist_free(&items);
    buf_free(&out);
  }

  TEST_REPORT("header");
}
