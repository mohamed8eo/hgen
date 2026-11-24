#include "source.h"

#include "buf.h"
#include "fileio.h"
#include "structs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Match a complete "base" or <base> token at the start of p. */
static int match_quoted(const char *p, size_t avail, const char *base,
                        char open, char close) {
  size_t n = strlen(base);
  if (avail < n + 2 || *p != open || memcmp(p + 1, base, n) != 0 ||
      p[n + 1] != close)
    return 0;

  const char *rest = p + n + 2;
  while (rest < p + avail && (*rest == ' ' || *rest == '\t' || *rest == '\r'))
    rest++;
  return rest == p + avail;
}

int source_has_own_include(const char *src, const char *h_path) {
  if (src == NULL || h_path == NULL)
    return -1;

  const char *sep = strrchr(h_path, '/');
  const char *base = sep != NULL ? sep + 1 : h_path;
  if (*base == '\0')
    return 0;

  const char *line = src;
  while (*line != '\0') {
    const char *nl = strchr(line, '\n');
    size_t len = nl != NULL ? (size_t)(nl - line) : strlen(line);
    const char *p = line;

    while (p < line + len && (*p == ' ' || *p == '\t'))
      p++;
    if (p < line + len && *p == '#') {
      p++;
      while (p < line + len && (*p == ' ' || *p == '\t'))
        p++;
      if ((size_t)(line + len - p) >= 7 && memcmp(p, "include", 7) == 0) {
        p += 7;
        while (p < line + len && (*p == ' ' || *p == '\t'))
          p++;
        size_t avail = (size_t)(line + len - p);
        if (match_quoted(p, avail, base, '"', '"') ||
            match_quoted(p, avail, base, '<', '>'))
          return 1;
      }
    }

    if (nl == NULL)
      break;
    line = nl + 1;
  }
  return 0;
}

/* Build a copy of src with the header include inserted. */
static int with_include(const char *src, const char *h_path, Buf *out) {
  const char *sep = strrchr(h_path, '/');
  const char *base = sep != NULL ? sep + 1 : h_path;

  char inc[512];
  int n = snprintf(inc, sizeof inc, "#include \"%s\"\n", base);
  if (n < 0 || (size_t)n >= sizeof inc)
    return -1;

  size_t ins = include_insert_pos(src);
  if (buf_append_n(out, src, ins) == -1 ||
      buf_append(out, inc) == -1 ||
      buf_append_n(out, src + ins, strlen(src) - ins) == -1)
    return -1;
  return 0;
}

/* Build a copy of src with the emitted struct declarations removed. */
static char *struct_removed(const char *src, const ItemList *list,
                            const unsigned char *flags) {
  Buf out = {0};
  size_t pos = 0;

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
    if (start > pos && buf_append_n(&out, src + pos, start - pos) == -1)
      goto fail;
    pos = end;
  }

  if (buf_append_n(&out, src + pos, strlen(src) - pos) == -1)
    goto fail;
  return out.data;

fail:
  buf_free(&out);
  return NULL;
}

int source_rewrite(const char *src, const ItemList *list,
                   const char *h_path, const char *c_path, int *added) {
  if (added != NULL)
    *added = 0;
  if (src == NULL || list == NULL || h_path == NULL || c_path == NULL)
    return -1;

  unsigned char *flags = struct_flags(src, list, NULL);
  if (list->len > 0 && flags == NULL)
    return -1;

  char *without = struct_removed(src, list, flags);
  free(flags);
  if (without == NULL)
    return -1;

  int have = source_has_own_include(without, h_path);
  if (have < 0) {
    free(without);
    return -1;
  }

  int rc;
  if (have == 1) {
    rc = write_file(c_path, without);
  } else {
    Buf final = {0};
    if (with_include(without, h_path, &final) == -1) {
      rc = -1;
    } else {
      rc = write_file(c_path, final.data);
      if (rc == 0 && added != NULL)
        *added = 1;
    }
    buf_free(&final);
  }

  free(without);
  return rc;
}

int source_ensure_include(const char *c_path, const char *h_path, int *added) {
  if (added != NULL)
    *added = 0;
  if (c_path == NULL || h_path == NULL)
    return -1;

  size_t len = 0;
  char *src = read_file(c_path, &len);
  if (src == NULL)
    return -1;

  int have = source_has_own_include(src, h_path);
  if (have != 0) {
    free(src);
    return have < 0 ? -1 : 0; /* 1 = already there, skip */
  }

  Buf out = {0};
  int rc = with_include(src, h_path, &out);
  if (rc == 0) {
    rc = write_file(c_path, out.data);
    if (rc == 0 && added != NULL)
      *added = 1;
  }

  buf_free(&out);
  free(src);
  return rc;
}
