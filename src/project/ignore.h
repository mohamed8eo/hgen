#ifndef IGNORE_H
#define IGNORE_H

#include "project.h"

#include <stddef.h>

typedef struct {
  char *pat;       /* pattern without trailing '/' */
  int   dir_only;  /* 1 if the line ended with '/' */
} IgnoreRule;

typedef struct {
  IgnoreRule *rules;
  size_t      len;
} IgnoreList;

int  ignore_load(const char *root, IgnoreList *out);
int  ignore_match(const IgnoreList *l, const char *name, int is_dir);
void ignore_free(IgnoreList *l);

#endif // !IGNORE_H
