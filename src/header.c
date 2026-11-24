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

/* --- struct declarations ---------------------------------------------- */

static int ident_char(char c) {
  return c == '_' || isalnum((unsigned char)c);
}

static char *dup_range(const char *begin, const char *end) {
  size_t len = (size_t)(end - begin);
  char *s = malloc(len + 1);
  if (s != NULL) {
    memcpy(s, begin, len);
    s[len] = '\0';
  }
  return s;
}

static char *slice_dup(const char *src, const Item *ite) {
  return dup_range(src + ite->start, src + ite->end);
}

/* A struct can be named by a tag, by a typedef alias, or by both. */
typedef struct {
  char *names[2];
  size_t len;
} Names;

static void names_add(Names *names, const char *begin, const char *end) {
  if (begin != end && names->len < 2)
    names->names[names->len++] = dup_range(begin, end);
}

static void names_free(Names *names) {
  for (size_t i = 0; i < names->len; i++)
    free(names->names[i]);
  names->len = 0;
}

/* Extract the type names introduced by a struct declaration. */
static void struct_names(const char *text, Names *out) {
  const char *p = text;
  while (isspace((unsigned char)*p))
    p++;

  int is_typedef = 0;
  if (strncmp(p, "typedef", 7) == 0 &&
      (isspace((unsigned char)p[7]) || p[7] == '\0')) {
    is_typedef = 1;
    p += 7;
    while (isspace((unsigned char)*p))
      p++;
  }

  if (strncmp(p, "struct", 6) != 0 ||
      !(isspace((unsigned char)p[6]) || p[6] == '\0'))
    return;
  p += 6;
  while (isspace((unsigned char)*p))
    p++;

  if (ident_char(*p)) {
    const char *q = p;
    while (ident_char(*q))
      q++;
    names_add(out, p, q); /* struct tag */
  }

  if (!is_typedef)
    return;

  /* typedef struct { ... } Alias; -- the alias precedes the final ';'. */
  const char *end = p + strlen(p);
  while (end > p && isspace((unsigned char)end[-1]))
    end--;
  if (end > p && end[-1] == ';') {
    end--;
    while (end > p && isspace((unsigned char)end[-1]))
      end--;
    const char *begin = end;
    while (begin > p && ident_char(begin[-1]))
      begin--;
    names_add(out, begin, end);
  }
}

static int contains_word(const char *text, const char *word) {
  size_t len = strlen(word);
  if (len == 0)
    return 0;

  for (const char *p = text; (p = strstr(p, word)) != NULL; p++) {
    if ((p == text || !ident_char(p[-1])) && !ident_char(p[len]))
      return 1;
  }
  return 0;
}

/* A struct is public when one of its names appears in a public prototype. */
static int struct_is_public(const char *text, const ItemList *list) {
  Names names = {0};
  struct_names(text, &names);

  int found = 0;
  for (size_t i = 0; i < names.len && !found; i++) {
    for (size_t j = 0; j < list->len; j++) {
      const Item *ite = &list->items[j];
      if (ite->kind == ITEM_FUNC &&
          contains_word(ite->text, names.names[i])) {
        found = 1;
        break;
      }
    }
  }

  names_free(&names);
  return found;
}

/* Buf's text lives in [start, end) of the stripped source, not in Item.text. */
static int append_slice(Buf *out, const char *begin, const char *end) {
  char *tmp = dup_range(begin, end);
  if (tmp == NULL) {
    perror("malloc");
    return -1;
  }
  int rc = buf_append(out, tmp);
  free(tmp);
  return rc;
}

/* One flag per item: true for structs that belong in the header. */
static unsigned char *struct_flags(const char *src, const ItemList *list,
                                   int *need_stddef) {
  unsigned char *flags = NULL;
  if (need_stddef != NULL)
    *need_stddef = 0;
  if (list->len == 0)
    return NULL;

  flags = calloc(list->len, sizeof *flags);
  if (flags == NULL) {
    perror("calloc");
    return NULL;
  }

  for (size_t i = 0; i < list->len; i++) {
    const Item *ite = &list->items[i];
    if (ite->kind != ITEM_STRUCT)
      continue;
    char *text = slice_dup(src, ite);
    if (text == NULL) {
      free(flags);
      return NULL;
    }
    flags[i] = struct_is_public(text, list);
    if (flags[i] && need_stddef != NULL && strstr(text, "size_t") != NULL)
      *need_stddef = 1;
    free(text);
  }
  return flags;
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
  if (need_stddef && buf_append(out, "#include <stddef.h>\n\n") == -1)
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
      if (!emit[i])
        break;
      if (append_slice(out, src + ite->start, src + ite->end) == -1)
        goto fail;
      if (buf_append(out, "\n\n") == -1)
        goto fail;
      break;
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

  free(emit);
  free(guard);
  return 0;

fail:
  free(emit);
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

/* Position just after the leading block of preprocessor lines. */
static size_t include_insert_pos(const char *src) {
  size_t line_start = 0;
  size_t pos = 0;

  while (src[line_start] != '\0') {
    const char *p = src + line_start;
    while (*p == ' ' || *p == '\t')
      p++;
    if (*p != '#')
      break;
    const char *e = strchr(p, '\n');
    if (e == NULL)
      return strlen(src);
    pos = (size_t)(e - src) + 1;
    line_start = pos;
  }
  return pos;
}

static int already_includes(const char *src, const char *base) {
  char needle[512];
  int n = snprintf(needle, sizeof needle, "#include \"%s\"", base);
  if (n < 0 || (size_t)n >= sizeof needle)
    return 0;
  if (strstr(src, needle) != NULL)
    return 1;
  n = snprintf(needle, sizeof needle, "#include <%s>", base);
  return n >= 0 && (size_t)n < sizeof needle && strstr(src, needle) != NULL;
}

/* Rebuild the .c with the moved structs removed and its header included. */
static char *rewritten_source(const char *src, const ItemList *list,
                              const unsigned char *flags, const char *h_path) {
  const char *sep = strrchr(h_path, '/');
  const char *base = sep != NULL ? sep + 1 : h_path;

  char inc[512];
  int n = snprintf(inc, sizeof inc, "#include \"%s\"\n", base);
  if (n < 0 || (size_t)n >= sizeof inc)
    return NULL;

  int have = already_includes(src, base);
  size_t ins = have ? 0 : include_insert_pos(src);

  Buf out = {0};
  size_t pos = 0;

  if (!have) {
    if (append_slice(&out, src, src + ins) == -1 ||
        buf_append(&out, inc) == -1)
      goto fail;
    pos = ins;
  }

  for (size_t i = 0; i < list->len; i++) {
    const Item *ite = &list->items[i];
    if (!flags[i])
      continue;

    size_t start = ite->start;
    size_t end = ite->end;
    if (src[end] == '\r' && src[end + 1] == '\n')
      end += 2;
    else if (src[end] == '\n')
      end += 1;

    /* Drop blank lines left behind by the removed declaration. */
    while (src[end] != '\0') {
      const char *p = src + end;
      while (*p == ' ' || *p == '\t')
        p++;
      if (p[0] == '\r' && p[1] == '\n')
        p += 2;
      else if (*p == '\n')
        p += 1;
      else
        break;
      end = (size_t)(p - src);
    }

    if (start < pos)
      start = pos;
    if (start > pos && append_slice(&out, src + pos, src + start) == -1)
      goto fail;
    pos = end;
  }

  if (append_slice(&out, src + pos, src + strlen(src)) == -1)
    goto fail;
  return out.data;

fail:
  buf_free(&out);
  return NULL;
}

int header_move_structs(const char *src, const ItemList *list,
                        const char *h_path, const char *c_path) {
  unsigned char *flags = struct_flags(src, list, NULL);
  if (list->len > 0 && flags == NULL)
    return -1;

  int any = 0;
  for (size_t i = 0; i < list->len; i++)
    any |= flags[i];
  if (!any) {
    free(flags);
    return 0;
  }

  char *text = rewritten_source(src, list, flags, h_path);
  free(flags);
  if (text == NULL)
    return -1;

  int rc = write_file(c_path, text);
  free(text);
  return rc;
}
