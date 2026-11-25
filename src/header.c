#include "header.h"
#include "buf.h"
#include "scan.h"
#include "structs.h"

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

/* Functions only. Safe to regenerate on every run. */
static int append_protos(const ItemList *list, Buf *out) {
  for (size_t i = 0; i < list->len; i++) {
    const Item *ite = &list->items[i];
    if (ite->kind != ITEM_FUNC)
      continue;
    if (buf_append(out, ite->text) == -1)
      return -1;
    if (buf_append(out, "\n") == -1)
      return -1;
  }
  return 0;
}

/* Public structs only. Used when CREATING the header, never on update. */
static int append_structs(const ItemList *list, const unsigned char *emit,
                          const char *src, Buf *out) {
  for (size_t i = 0; i < list->len; i++) {
    const Item *ite = &list->items[i];
    if (ite->kind != ITEM_STRUCT || !emit[i])
      continue;
    if (buf_append_n(out, src + ite->start, ite->end - ite->start) == -1)
      return -1;
    if (buf_append(out, "\n\n") == -1)
      return -1;
  }
  return 0;
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
  char *guard = guard_name(h_path);
  if (guard == NULL)
    return -1;

  /* Decide which structs go into the header before writing anything, so the
     stddef include (needed for size_t fields) can be placed at the top. */
  int need_stddef = 0;
  unsigned char *emit = struct_flags(src, list, &need_stddef);
  if (list->len > 0 && emit == NULL) {
    free(guard);
    return -1;
  }

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

  if (need_stddef && buf_append(out, "#include <stddef.h>\n\n") == -1)
    goto fail;

  if (append_structs(list, emit, src, out) == -1) /* ABOVE the markers */
    goto fail;

  if (buf_append(out, BEGIN_MARK) == -1)
    goto fail;

  if (append_protos(list, out) == -1) /* BETWEEN the markers */
    goto fail;

  if (buf_append(out, END_MARK) == -1)
    goto fail;

  if (buf_append(out, "\n#endif /* ") == -1)
    goto fail;

  if (buf_append(out, guard) == -1)
    goto fail;

  if (buf_append(out, " */\n") == -1)
    goto fail;

  free(emit);
  free(guard);
  return 0;

fail:
  free(emit);
  free(guard);
  return -1;
}

/* Return the last END marker at or after "from", or NULL. Old headers may
   contain several of them (a generator bug), and all but one must go away. */
static const char *last_end_marker(const char *from) {
  const size_t n = strlen(END_MARK);
  const char *last = NULL;
  const char *p = from;

  while ((p = strstr(p, END_MARK)) != NULL) {
    last = p;
    p += n;
  }
  return last;
}

int header_update(const char *old_text, const ItemList *list, Buf *out) {
  if (old_text == NULL || list == NULL || out == NULL) {
    fprintf(stderr, "invalid arguments to header_update\n");
    return -1;
  }

  const char *b = strstr(old_text, BEGIN_MARK);
  if (b == NULL) {
    fprintf(stderr, "BEGIN marker not found; refusing to touch the header\n");
    return -1;
  }

  const char *after_begin = b + strlen(BEGIN_MARK);
  /* Search only after BEGIN: an END marker before it is not our block. */
  const char *last_e = last_end_marker(after_begin);
  if (last_e == NULL) {
    fprintf(stderr, "END marker not found after BEGIN; refusing to touch the header\n");
    return -1;
  }

  /* Everything above the markers (guard, includes, structs) and everything
     below them (#endif) is user-owned text: keep it exactly as it was. The
     generated block is replaced wholesale, including stray extra END markers
     from older runs, so exactly one END marker is emitted. */

  if (buf_append_n(out, old_text, (size_t)(after_begin - old_text)) == -1)
    return -1;
  if (append_protos(list, out) == -1)
    return -1;
  if (buf_append(out, END_MARK) == -1)
    return -1;
  if (buf_append(out, last_e + strlen(END_MARK)) == -1)
    return -1;

  return 0;
}
