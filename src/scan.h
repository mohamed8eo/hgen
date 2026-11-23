#ifndef SCAN_H
#define SCAN_H

#include <stddef.h>
#include <stdio.h>

typedef enum { ITEM_FUNC, ITEM_STRUCT, ITEM_SKIP } Kind;

typedef struct {
  Kind kind;
  size_t start, end; /* byte range in the stripped source: [start, end) */
  char *text;        // Only contain the function signature.
} Item;

typedef struct {
  Item *items;
  size_t len;
  size_t cap;
} ItemList;

/* Lists must be zero-initialized (e.g. ItemList list = {0};). */
void itemlist_free(ItemList *list);
void scan(const char *src, ItemList *out);

#endif
