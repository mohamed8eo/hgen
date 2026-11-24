#include "structs.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

unsigned char *struct_flags(const char *src, const ItemList *list,
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
