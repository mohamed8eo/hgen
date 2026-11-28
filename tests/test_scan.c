#include "test.h"

/* Include the implementation to reach the file-local helpers. */
#include "scan/scan.c"

static char *dup_text(const char *s) {
  size_t n = strlen(s) + 1;
  char *copy = malloc(n);
  if (copy != NULL)
    memcpy(copy, s, n);
  return copy;
}

static void check_slice(const char *src, const Item *item, const char *want) {
  size_t n = item->end - item->start;
  char *slice = malloc(n + 1);
  CHECK(slice != NULL);
  if (slice == NULL)
    return;
  memcpy(slice, src + item->start, n);
  slice[n] = '\0';
  CHECK_STR(slice, want);
  free(slice);
}

int main(void) {
  /* --- is_space ------------------------------------------------------- */
  CHECK(is_space(' '));
  CHECK(is_space('\t'));
  CHECK(is_space('\n'));
  CHECK(is_space('\r'));
  CHECK(!is_space('a'));
  CHECK(!is_space('\0'));
  CHECK(!is_space(','));

  /* --- starts_with_word: the three bug rows, plus boundaries ---------- */
  CHECK_INT(starts_with_word("str x", "struct"), 0);
  CHECK_INT(starts_with_word("struct", "struct"), 1);
  CHECK_INT(starts_with_word("struct kind ", "struct"), 1);
  CHECK_INT(starts_with_word("struct", "structs"), 0);
  CHECK_INT(starts_with_word("structs ", "struct"), 0);
  CHECK_INT(starts_with_word("structure ", "struct"), 0);
  CHECK_INT(starts_with_word("struct\nkind", "struct"), 1);
  CHECK_INT(starts_with_word("struct\t", "struct"), 1);
  CHECK_INT(starts_with_word("xstruct ", "struct"), 0);
  CHECK_INT(starts_with_word("", "struct"), 0);
  CHECK_INT(starts_with_word("static void f(void);", "static"), 1);
  CHECK_INT(starts_with_word("staticx ", "static"), 0);

  /* --- is_typedef_struct ---------------------------------------------- */
  CHECK(is_typedef_struct("typedef struct "));
  CHECK(is_typedef_struct("typedef struct"));
  CHECK(is_typedef_struct("typedef struct Foo "));
  CHECK(!is_typedef_struct("typedef enum "));
  CHECK(!is_typedef_struct("typedef "));
  CHECK(!is_typedef_struct("typedef structx "));
  CHECK(!is_typedef_struct("struct Foo "));
  CHECK(!is_typedef_struct(""));

  /* --- clean_signature ------------------------------------------------ */
  {
    char sig[128] = "void *buf_push(Buf *b,\n const char *s) ";
    clean_signature(sig);
    CHECK_STR(sig, "void *buf_push(Buf *b, const char *s);");
  }
  {
    char sig[128] = "  void f(void)  \n";
    clean_signature(sig);
    CHECK_STR(sig, "void f(void);");
  }
  {
    char sig[128] = "int  f( int   a ) ";
    clean_signature(sig);
    CHECK_STR(sig, "int f( int a );");
  }
  {
    char sig[128] = "int f(void)";
    clean_signature(sig);
    CHECK_STR(sig, "int f(void);");
  }
  {
    char sig[128] = "int x  "; /* no '(' means untouched */
    clean_signature(sig);
    CHECK_STR(sig, "int x  ");
  }

  /* --- is_ident_char / is_main ---------------------------------------- */
  CHECK(is_ident_char('_'));
  CHECK(is_ident_char('a'));
  CHECK(is_ident_char('Z'));
  CHECK(is_ident_char('0'));
  CHECK(!is_ident_char('-'));
  CHECK(!is_ident_char(' '));
  CHECK(!is_ident_char('\0'));
  CHECK(!is_ident_char('('));

  CHECK(is_main("int main(void);"));
  CHECK(is_main("int main (void);"));
  CHECK(is_main("void\nmain(void);"));
  CHECK(!is_main("int f(void);"));
  CHECK(!is_main("int mymain(void);"));
  CHECK(!is_main("int main2(void);"));
  CHECK(!is_main("int (*fp)(void);"));
  CHECK(!is_main(""));
  CHECK(!is_main("("));

  /* --- classify: decides only, never modifies ------------------------- */
  {
    char sig[] = "int  f( void )  ";
    CHECK(classify(sig) == ITEM_FUNC);
    CHECK_STR(sig, "int  f( void )  ");
  }
  {
    char decl[] = "typedef enum";
    CHECK(classify(decl) == ITEM_SKIP);
  }
  {
    char decl[] = "struct Foo ";
    CHECK(classify(decl) == ITEM_STRUCT);
  }
  {
    char decl[] = "typedef struct ";
    CHECK(classify(decl) == ITEM_STRUCT);
  }
  {
    char decl[] = "int x = 1";
    CHECK(classify(decl) == ITEM_SKIP);
  }
  {
    char decl[] = "static void f(void) ";
    CHECK(classify(decl) == ITEM_FUNC); /* scan does the static demotion */
  }

  /* --- scan: struct ranges end at the statement's ';' ------------------ */
  {
    const char *src = "typedef struct {\n  int x;\n} Foo;\n";
    ItemList list = {0};
    scan(src, &list);
    CHECK_INT(list.len, 1);
    if (list.len == 1) {
      CHECK(list.items[0].kind == ITEM_STRUCT);
      CHECK_SIZE(list.items[0].end, strlen(src) - 1);
      check_slice(src, &list.items[0], "typedef struct {\n  int x;\n} Foo;");
    }
    itemlist_free(&list);
  }
  {
    const char *src = "struct { int x; } v;\n";
    ItemList list = {0};
    scan(src, &list);
    CHECK_INT(list.len, 1);
    if (list.len == 1) {
      CHECK(list.items[0].kind == ITEM_STRUCT);
      check_slice(src, &list.items[0], "struct { int x; } v;");
    }
    itemlist_free(&list);
  }
  {
    const char *src = "struct A {\n  int x;\n  struct B { int y; };\n};\n";
    ItemList list = {0};
    scan(src, &list);
    CHECK_INT(list.len, 1);
    if (list.len == 1)
      check_slice(src, &list.items[0],
                  "struct A {\n  int x;\n  struct B { int y; };\n};");
    itemlist_free(&list);
  }

  /* --- scan: static helpers and main are skipped ----------------------- */
  {
    const char *src = "static void helper(void) {}\n"
                      "int main(void) { return 0; }\n"
                      "int pub(void) { return 1; }\n";
    ItemList list = {0};
    scan(src, &list);
    CHECK_INT(list.len, 3);
    if (list.len == 3) {
      CHECK(list.items[0].kind == ITEM_SKIP);
      CHECK(list.items[1].kind == ITEM_SKIP);
      CHECK(list.items[2].kind == ITEM_FUNC);
      CHECK_STR(list.items[2].text, "int pub(void);");
    }
    itemlist_free(&list);
  }

  /* --- scan: functions, preprocessor lines, empty input --------------- */
  {
    const char *src = "int f(void) {\n  int x = 1;\n  return x;\n}\n";
    ItemList list = {0};
    scan(src, &list);
    CHECK_INT(list.len, 1);
    if (list.len == 1) {
      CHECK(list.items[0].kind == ITEM_FUNC);
      CHECK_STR(list.items[0].text, "int f(void);");
    }
    itemlist_free(&list);
  }
  {
    const char *src = "#define FAKE {\nstruct A { int x; };\n";
    ItemList list = {0};
    scan(src, &list);
    CHECK_INT(list.len, 1);
    if (list.len == 1)
      check_slice(src, &list.items[0], "struct A { int x; };");
    itemlist_free(&list);
  }
  {
    /* A backslash at the end of a directive continues it onto the next line;
       the continued text must not be scanned as C. */
    const char *src = "#define LEAK \\\n"
                      "void leaked(void) {}\n"
                      "struct A { int x; };\n";
    ItemList list = {0};
    scan(src, &list);
    CHECK_INT(list.len, 1);
    if (list.len == 1)
      check_slice(src, &list.items[0], "struct A { int x; };");
    itemlist_free(&list);
  }
  {
    const char *src = "#define LEAK \\\n"
                      "struct B { int y; }; \\\n"
                      "int leaked(void) { return 0; }\n";
    ItemList list = {0};
    scan(src, &list);
    CHECK_INT(list.len, 0);
    CHECK(list.items == NULL);
    itemlist_free(&list);
  }
  {
    ItemList list = {0};
    scan("", &list);
    CHECK_INT(list.len, 0);
    CHECK(list.items == NULL);
    itemlist_free(&list);
  }
  {
    const char *src = "struct A { int x; };\n"
                      "static void h(void) {}\n"
                      "int f(void) { return 0; }\n"
                      "int main(void) { return 1; }\n";
    ItemList list = {0};
    scan(src, &list);
    CHECK_INT(list.len, 4);
    if (list.len == 4) {
      CHECK(list.items[0].kind == ITEM_STRUCT);
      CHECK(list.items[1].kind == ITEM_SKIP);
      CHECK(list.items[2].kind == ITEM_FUNC);
      CHECK(list.items[3].kind == ITEM_SKIP);
    }
    itemlist_free(&list);
  }

  /* --- itemlist_push / itemlist_free ---------------------------------- */
  {
    ItemList list = {0};
    for (int i = 0; i < 10; i++) {
      Item item = {.kind = ITEM_FUNC,
                   .start = (size_t)i,
                   .end = (size_t)i + 1,
                   .text = dup_text("f(void);")};
      itemlist_push(&list, item);
    }
    CHECK_INT(list.len, 10);
    CHECK(list.cap >= list.len);
    CHECK_STR(list.items[9].text, "f(void);");
    itemlist_free(&list);
    CHECK_INT(list.len, 0);
    CHECK_SIZE(list.cap, 0);
    CHECK(list.items == NULL);
    itemlist_free(&list); /* safe on an already-empty list */
  }

  TEST_REPORT("scan");
}
