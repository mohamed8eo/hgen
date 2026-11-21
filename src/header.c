#include "header.h"
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Build a valid C identifier from the basename of a .h path:
   "dir/kmp.h" -> "HGEN_KMP_H". Caller owns the result. */
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

  printf("FILE Header: %s\n", guard);
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
  printf("Header c_path: %s\n", h_path);

  char *guard = guard_name(h_path);
  if (guard == NULL) {
    free(h_path);
    return NULL;
  }
  free(guard);

  return h_path;
}
