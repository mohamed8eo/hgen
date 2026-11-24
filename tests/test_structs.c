#include "test.h"

#include "scan.h"
#include "structs.h"

int main(void) {
  /* Public typedef is emitted; private struct and functions are not. */
  {
    const char *src = "typedef struct {\n"
                      "  int x;\n"
                      "} Pub;\n"
                      "struct Hidden { int y; };\n"
                      "Pub *make(void) { return 0; }\n";
    ItemList items = {0};
    int need = -1;
    scan(src, &items);
    unsigned char *flags = struct_flags(src, &items, &need);

    CHECK_INT(items.len, 3);
    CHECK(flags != NULL);
    if (flags != NULL) {
      CHECK_INT(flags[0], 1);
      CHECK_INT(flags[1], 0);
      CHECK_INT(flags[2], 0);
    }
    CHECK_INT(need, 0);

    free(flags);
    itemlist_free(&items);
  }

  /* size_t in an emitted struct is reported to the header builder. */
  {
    const char *src = "typedef struct {\n"
                      "  size_t n;\n"
                      "} B;\n"
                      "B *make(void) { return 0; }\n";
    ItemList items = {0};
    int need = -1;
    scan(src, &items);
    unsigned char *flags = struct_flags(src, &items, &need);

    CHECK_INT(items.len, 2);
    CHECK(flags != NULL);
    if (flags != NULL) {
      CHECK_INT(flags[0], 1);
      CHECK_INT(flags[1], 0);
    }
    CHECK_INT(need, 1);

    free(flags);
    itemlist_free(&items);
  }

  /* No structs at all still yields a zeroed flag array for the items. */
  {
    const char *src = "int f(void) { return 0; }\n";
    ItemList items = {0};
    int need = -1;
    scan(src, &items);
    unsigned char *flags = struct_flags(src, &items, &need);

    CHECK_INT(items.len, 1);
    CHECK(flags != NULL);
    if (flags != NULL)
      CHECK_INT(flags[0], 0);
    CHECK_INT(need, 0);

    free(flags);
    itemlist_free(&items);
  }

  /* An empty item list returns NULL rather than allocating. */
  {
    ItemList empty = {0};
    int need = -1;
    CHECK(struct_flags("", &empty, &need) == NULL);
    CHECK_INT(need, 0);
  }

  TEST_REPORT("structs");
}
