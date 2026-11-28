#define _DEFAULT_SOURCE

#include "ignore.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

/* Universal defaults: names almost nobody wants scanned, in any project. */
static const char *DEFAULTS[] = {".git", ".hgen", "build", "node_modules"};
#define DEFAULT_COUNT (sizeof(DEFAULTS) / sizeof(DEFAULTS[0]))

/* Append a rule to the list, growing the array with realloc. */
static int push_rule(IgnoreList *list, const char *pat, int dir_only) {
  IgnoreRule *tmp = realloc(list->rules, (list->len + 1) * sizeof(*tmp));
  if (tmp == NULL)
    return -1;
  list->rules = tmp;
  list->rules[list->len].pat = strdup(pat);
  if (list->rules[list->len].pat == NULL)
    return -1;
  list->rules[list->len].dir_only = dir_only;
  list->len++;
  return 0;
}

/* Load the built-in defaults through the same push path. */
static int load_defaults(IgnoreList *out) {
  for (size_t i = 0; i < DEFAULT_COUNT; i++)
    if (push_rule(out, DEFAULTS[i], 0) != 0)
      return -1;
  return 0;
}

/* Return a malloc'd copy of line with trailing whitespace (\r, space, tab,
 * newline) trimmed. Caller frees. */
static char *trim_copy(const char *line) {
  size_t n = strlen(line);
  while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r' ||
                   line[n - 1] == ' '  || line[n - 1] == '\t'))
    n--;
  char *out = malloc(n + 1);
  if (out == NULL)
    return NULL;
  memcpy(out, line, n);
  out[n] = '\0';
  return out;
}

/* Glob matcher: * matches any run of characters (including none).
 * Recursion tries the rest of the pattern against every suffix of s. */
static int glob_match(const char *pat, const char *s) {
  if (*pat == '\0')
    return *s == '\0';

  if (*pat == '*') {
    const char *rest = pat + 1;
    for (const char *p = s; ; p++) {
      if (glob_match(rest, p))
        return 1;
      if (*p == '\0')
        break;
    }
    return 0;
  }

  if (*pat != *s)
    return 0;
  return glob_match(pat + 1, s + 1);
}

int ignore_load(const char *root, IgnoreList *out) {
  out->rules = NULL;
  out->len = 0;

  /* NULL root means defaults, no file access at all. */
  if (root == NULL)
    return load_defaults(out);

  char *path = NULL;
  int n = snprintf(NULL, 0, "%s/%s", root, HGENIGNORE) + 1;
  if (n < 0) {
    return -1;
  }
  path = malloc((size_t)n);
  if (path == NULL)
    return -1;
  snprintf(path, (size_t)n, "%s/%s", root, HGENIGNORE);

  FILE *fp = fopen(path, "r");
  free(path);

  if (fp == NULL) {
    /* Missing file is not an error: use built-in defaults, write nothing.
     * Any other error (permissions, etc.) is reported. */
    if (errno != ENOENT) {
      perror("fopen");
      return -1;
    }
    return load_defaults(out);
  }

  char *line = NULL;
  size_t cap = 0;
  ssize_t len;
  int rc = 0;

  while ((len = getline(&line, &cap, fp)) != -1) {
    char *trimmed = trim_copy(line);
    if (trimmed == NULL) {
      rc = -1;
      break;
    }

    /* Skip blank lines and comments. */
    if (trimmed[0] == '\0' || trimmed[0] == '#') {
      free(trimmed);
      continue;
    }

    int dir_only = 0;
    size_t tlen = strlen(trimmed);
    if (tlen > 0 && trimmed[tlen - 1] == '/') {
      dir_only = 1;
      trimmed[--tlen] = '\0';
    }

    /* A lone "/" (or whitespace-only line trimmed to empty) is ignored. */
    if (tlen == 0) {
      free(trimmed);
      continue;
    }

    if (push_rule(out, trimmed, dir_only) != 0) {
      rc = -1;
      free(trimmed);
      break;
    }
    free(trimmed);
  }

  free(line);
  fclose(fp);

  if (rc != 0) {
    ignore_free(out);
    return -1;
  }
  return 0;
}

int ignore_match(const IgnoreList *l, const char *name, int is_dir) {
  for (size_t i = 0; i < l->len; i++) {
    const IgnoreRule *r = &l->rules[i];
    if (r->dir_only && !is_dir)
      continue;
    if (glob_match(r->pat, name))
      return 1;
  }
  return 0;
}

void ignore_free(IgnoreList *l) {
  for (size_t i = 0; i < l->len; i++)
    free(l->rules[i].pat);
  free(l->rules);
  l->rules = NULL;
  l->len = 0;
}
