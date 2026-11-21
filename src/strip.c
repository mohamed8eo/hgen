#include "strip.h"

#include <stdlib.h>
#include <string.h>

enum state {
  NORMAL,
  LINE_COMMENT,
  BLOCK_COMMENT,
  STRING,
  CHAR,
};

char *strip_comments_and_literals(const char *src) {
  size_t len = strlen(src);
  char *result = malloc(len + 1);
  if (result == NULL) {
    return NULL;
  }

  enum state state = NORMAL;
  for (size_t i = 0; i < len; i++) {
    char c = src[i];
    char next = (i + 1 < len) ? src[i + 1] : '\0';

    switch (state) {
    case NORMAL:
      if (c == '/' && next == '/') {
        result[i] = ' ';
        result[i + 1] = ' ';
        i++;
        state = LINE_COMMENT;
      } else if (c == '/' && next == '*') {
        result[i] = ' ';
        result[i + 1] = ' ';
        i++;
        state = BLOCK_COMMENT;
      } else if (c == '"') {
        result[i] = ' ';
        state = STRING;
      } else if (c == '\'') {
        result[i] = ' ';
        state = CHAR;
      } else {
        result[i] = c;
      }
      break;

    case LINE_COMMENT:
      if (c == '\n') {
        result[i] = '\n';
        state = NORMAL;
      } else {
        result[i] = ' ';
      }
      break;

    case BLOCK_COMMENT:
      if (c == '*' && next == '/') {
        result[i] = ' ';
        result[i + 1] = ' ';
        i++;
        state = NORMAL;
      } else if (c == '\n') {
        result[i] = '\n';
      } else {
        result[i] = ' ';
      }
      break;

    case STRING:
      if (c == '\\' && i + 1 < len) {
        result[i] = ' ';
        result[i + 1] = (src[i + 1] == '\n') ? '\n' : ' ';
        i++;
      } else if (c == '"') {
        result[i] = ' ';
        state = NORMAL;
      } else if (c == '\n') {
        result[i] = '\n';
      } else {
        result[i] = ' ';
      }
      break;

    case CHAR:
      if (c == '\\' && i + 1 < len) {
        result[i] = ' ';
        result[i + 1] = (src[i + 1] == '\n') ? '\n' : ' ';
        i++;
      } else if (c == '\'') {
        result[i] = ' ';
        state = NORMAL;
      } else if (c == '\n') {
        result[i] = '\n';
      } else {
        result[i] = ' ';
      }
      break;
    }
  }
  result[len] = '\0';
  return result;
}