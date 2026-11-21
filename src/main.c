#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fileio.h"
#include "header.h"
#include "scan.h"
#include "strip.h"

static void print_items(const ItemList *list, const char *src) {
  for (size_t i = 0; i < list->len; i++) {
    const Item *item = &list->items[i];
    switch (item->kind) {
    case ITEM_FUNC:
      printf("FUNC %s\n", item->text);
      break;
    case ITEM_STRUCT:
      printf("STRUCT %.*s\n", (int)(item->end - item->start),
             src + item->start);
      break;
    case ITEM_SKIP:
      break; /* skipped declarations are not shown */
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s file.c\n", argv[0]);
    return EXIT_FAILURE;
  }

  size_t out_len;
  const char *path = argv[1];
  size_t path_len = strlen(path);
  char *buf = read_file(path, &out_len);
  if (buf == NULL) {
    perror("read_file");
    return EXIT_FAILURE;
  }

  char *result = strip_comments_and_literals(buf);
  if (result == NULL) {
    perror("strip_comments_and_literals");
    free(buf);
    return EXIT_FAILURE;
  }

  ItemList items = {0};
  scan(result, &items);
  print_items(&items, buf);
  itemlist_free(&items);

  char *h_path = header_path(path, path_len);
  if (h_path == NULL) {
    free(buf);
    free(result);
    return EXIT_FAILURE;
  }
  free(h_path);

  free(buf);
  free(result);
  return EXIT_SUCCESS;
}
