#include "scan.h"
#include <stdio.h>

static int is_space(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

void scan(const char *src) {
  int depth = 0;
  const char *start = src;

  for (const char *p = src; *p != '\0'; p++) {
    // p[-1] == *(p-1)
    if (depth == 0 && (p == src || p[-1] == '\n') && *p == '#') {
      while (*p != '\0' && *p != '\n')
        p++;
      if (*p == '\n')
        start = p + 1;
      continue;
    }

    if (depth == 0 && start < p && is_space(*start) && !is_space(*p))
      start = p;

    if (*p == '{') {
      if (depth == 0) {
        printf("%.*s\n", (int)(p - start), start);
        start = p + 1;
      }
      depth++;
    } else if (*p == '}') {
      if (--depth == 0)
        start = p + 1;
    } else if (depth == 0 && *p == ';')
      start = p + 1;
  }
}
