#ifndef BUF_H
#define BUF_H
#include <stdlib.h>

typedef struct {
  char *data; // the text so far, always ends with '\0'
  size_t len; // characters used (not counting the '\0')
  size_t cap; // bytes allocated
} Buf;

int buf_append(Buf *b, const char *s); // 0 ok, -1 if realloc fails
int buf_append_n(Buf *b, const char *s, size_t n);
void buf_free(Buf *b);

#endif
