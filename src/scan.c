#include "scan.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_space(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/* Whole-word prefix test: all of word must match, and the next character in
   tar must end the token (end of string or whitespace). Bounding the loop by
   word instead of tar also avoids reading past tar's NUL. */
static int starts_with_word(const char *tar, const char *word) {
  size_t i = 0;

  while (word[i] != '\0') {
    if (tar[i] != word[i])
      return 0;
    i++;
  }

  return tar[i] == '\0' || is_space(tar[i]);
}

/* A typedef is a struct declaration only when "struct" follows it. */
static int is_typedef_struct(const char *s) {
  if (!starts_with_word(s, "typedef"))
    return 0;

  s += strlen("typedef");
  while (is_space(*s))
    s++;

  return starts_with_word(s, "struct");
}

/* Normalize whitespace across the whole declaration, trim it, and turn it
   into a statement. tar needs room for one extra byte for the ';'. */
static void clean_signature(char *tar) {
  if (strchr(tar, '(') == NULL)
    return;

  const char *read = tar;
  char *write = tar;
  int pending_space = 0;

  while (isspace((unsigned char)*read))
    read++;

  for (; *read != '\0'; read++) {
    if (isspace((unsigned char)*read)) {
      pending_space = 1;
    } else {
      if (write > tar && pending_space)
        *write++ = ' ';
      pending_space = 0;
      *write++ = *read;
    }
  }

  /* Trailing whitespace is dropped because pending_space is never flushed. */
  *write++ = ';';
  *write = '\0';
}

static int is_ident_char(char c) {
  return c == '_' || isalnum((unsigned char)c);
}

/* The function name is the identifier immediately before the first '('. */
static int is_main(const char *sig) {
  const char *lp = strchr(sig, '(');
  if (lp == NULL || lp == sig)
    return 0;

  const char *end = lp;
  while (end > sig && is_space(end[-1]))
    end--;

  const char *begin = end;
  while (begin > sig && is_ident_char(begin[-1]))
    begin--;

  size_t len = (size_t)(end - begin);
  return len == 4 && memcmp(begin, "main", 4) == 0;
}

static Kind classify(const char *tar) {
  if (strchr(tar, '='))
    return ITEM_SKIP;
  if (strchr(tar, '('))
    return ITEM_FUNC;
  if (starts_with_word(tar, "struct") || is_typedef_struct(tar))
    return ITEM_STRUCT;
  return ITEM_SKIP;
}

static void itemlist_push(ItemList *list, Item item) {
  if (list->len == list->cap) {
    size_t cap = list->cap == 0 ? 8 : list->cap * 2;
    Item *items = realloc(list->items, cap * sizeof *items);
    if (items == NULL) {
      perror("realloc");
      exit(EXIT_FAILURE);
    }
    list->items = items;
    list->cap = cap;
  }
  list->items[list->len++] = item;
}

void itemlist_free(ItemList *list) {
  for (size_t i = 0; i < list->len; i++)
    free(list->items[i].text);
  free(list->items);
  list->items = NULL;
  list->len = 0;
  list->cap = 0;
}

void scan(const char *src, ItemList *out) {
  int depth = 0;
  const char *start = src;
  size_t struct_item = (size_t)-1; /* index of the struct awaiting its ';' */

  for (const char *p = src; *p != '\0'; p++) {
    // p[-1] == *(p-1)
    if (depth == 0 && (p == src || p[-1] == '\n') && *p == '#') {
      /* Skip the whole directive, including any continued lines. Without
         this, the text after a trailing backslash is scanned as ordinary C
         and leaks fake declarations into the header. */
      const char *line = p;
      for (;;) {
        const char *e = strchr(line, '\n');
        size_t len = (e != NULL) ? (size_t)(e - line) : strlen(line);
        if (len > 0 && line[len - 1] == '\r')
          len--;
        if (len == 0 || line[len - 1] != '\\' || e == NULL) {
          p = (e != NULL) ? e : line + len;
          break;
        }
        line = e + 1;
      }
      if (*p == '\0')
        break;
      start = p + 1;
      continue;
    }

    if (depth == 0 && start < p && is_space(*start) && !is_space(*p))
      start = p;

    if (*p == '{') {
      if (depth == 0) {
        size_t len = (size_t)(p - start);
        char *tar = malloc(len + 2); /* +1 for NUL, +1 for ';' */
        if (tar == NULL) {
          perror("malloc");
          exit(EXIT_FAILURE);
        }
        memcpy(tar, start, len);
        tar[len] = '\0';

        Kind kind = classify(tar);
        if (kind == ITEM_FUNC) {
          clean_signature(tar);
          if (starts_with_word(tar, "static") || is_main(tar))
            kind = ITEM_SKIP;
        }

        Item item = {
            .kind = kind,
            .start = (size_t)(start - src),
            .end = (size_t)(p - src), /* refined at the statement's ';' */
            .text = tar,
        };
        itemlist_push(out, item);
        if (kind == ITEM_STRUCT)
          struct_item = out->len - 1;
        start = p + 1;
      }
      depth++;
    } else if (*p == '}') {
      if (--depth == 0)
        start = p + 1; /* the ';' still belongs to the statement */
    } else if (depth == 0 && *p == ';') {
      if (struct_item != (size_t)-1) {
        out->items[struct_item].end = (size_t)(p - src) + 1;
        struct_item = (size_t)-1;
      }
      start = p + 1;
    }
  }
}
