#include "buf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int buf_append_n(Buf *b, const char *s, size_t n) {
  size_t needed = b->len + n + 1;

  if (needed > b->cap) {
    size_t cap = b->cap == 0 ? 8 : b->cap;
    while (cap < needed)
      cap *= 2;

    char *buf = realloc(b->data, cap * sizeof(*b->data));
    if (buf == NULL) {
      perror("realloc");
      return -1;
    }

    b->data = buf;
    b->cap = cap;
  }
  memcpy(b->data + b->len, s, n);
  b->len += n;
  b->data[b->len] = '\0';

  return 0;
}

int buf_append(Buf *b, const char *s) {
  return buf_append_n(b, s, strlen(s));
}

void buf_free(Buf *b) {
  free(b->data);
  b->data = NULL;
  b->len = 0;
  b->cap = 0;
}
