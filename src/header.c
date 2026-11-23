#include "header.h"
#include "buf.h"
#include "scan.h"
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BEGIN_MARK                                                             \
  "/* --- BEGIN PROTOTYPES (auto-generated, do not edit) --- */\n"
#define END_MARK "/* --- END PROTOTYPES --- */\n"

/* Build a valid C identifier from the basename of a .h path:
   "dir/kmp.h" -> "HGEN_KMP_H". Caller owns the result.
   The HGEN_ prefix is deliberate: it avoids collisions with user guards and
   keeps the identifier valid even when the filename starts with a digit. */
static char *guard_name(const char *h_path) {
  if (h_path == NULL)
    return NULL;

  const char *separator = strrchr(h_path, '/');
  const char *filename = separator != NULL ? separator + 1 : h_path;
  size_t name_len = strlen(filename);

  const char prefix[] = "HGEN_";
  const size_t prefix_len = sizeof(prefix) - 1;

  char *guard = malloc(prefix_len + name_len + 1);
  if (guard == NULL) {
    perror("malloc");
    return NULL;
  }

  memcpy(guard, prefix, prefix_len);
  for (size_t i = 0; i < name_len; i++) {
    unsigned char c = (unsigned char)filename[i];
    guard[prefix_len + i] = isalnum(c) ? (char)toupper(c) : '_';
  }
  guard[prefix_len + name_len] = '\0';

  return guard;
}

char *header_path(const char *c_path, size_t path_len) {
  /* Validate before any pointer arithmetic or allocation. */
  if (c_path == NULL || path_len < 2 || path_len == SIZE_MAX ||
      c_path[path_len - 1] != 'c' || c_path[path_len - 2] != '.') {
    fprintf(stderr, "Expected a .c file\n");
    return NULL;
  }

  char *h_path = malloc(path_len + 1);
  if (h_path == NULL) {
    perror("malloc");
    return NULL;
  }

  /* Copy exactly path_len bytes from the caller and add our own NUL, so a
     length that lies about the source cannot cause an over-read. */
  memcpy(h_path, c_path, path_len);
  h_path[path_len] = '\0';
  h_path[path_len - 1] = 'h';

  return h_path;
}

int header_build(const char *src, const char *h_path, const ItemList *list,
                 Buf *out) {
  (void)src; /* needed by a later chapter */

  char *guard = guard_name(h_path);
  if (guard == NULL)
    return -1;

  // header
  if (buf_append(out, "#ifndef ") == -1)
    goto fail;
  if (buf_append(out, guard) == -1)
    goto fail;
  if (buf_append(out, "\n#define ") == -1)
    goto fail;
  if (buf_append(out, guard) == -1)
    goto fail;
  if (buf_append(out, "\n\n") == -1)
    goto fail;
  if (buf_append(out, BEGIN_MARK) == -1)
    goto fail;

  for (size_t i = 0; i < list->len; i++) {
    const Item *ite = &list->items[i];
    switch (ite->kind) {
    case ITEM_FUNC:
      if (buf_append(out, ite->text) == -1)
        goto fail;
      if (buf_append(out, "\n") == -1)
        goto fail;
      break;
    case ITEM_STRUCT:
    case ITEM_SKIP:
      break;
    }
  }

  if (buf_append(out, END_MARK) == -1)
    goto fail;
  if (buf_append(out, "\n#endif /* ") == -1)
    goto fail;
  if (buf_append(out, guard) == -1)
    goto fail;
  if (buf_append(out, " */\n") == -1)
    goto fail;

  free(guard);
  return 0;

fail:
  free(guard);
  return -1;
}

int write_file(const char *path, const char *text) {
  FILE *fp = fopen(path, "w");
  if (fp == NULL) {
    perror("fopen");
    return -1;
  }

  if (fputs(text, fp) == EOF) {
    perror("fputs");
    fclose(fp);
    return -1;
  }

  if (fclose(fp) != 0) {
    perror("fclose");
    return -1;
  }

  return 0;
}
