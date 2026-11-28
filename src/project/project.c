#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include "project.h"
#include "../project/ignore.h"
#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int ends_with_c(const char *name) {
  const char *last = strrchr(name, '.');
  if (last == NULL || last == name)
    return 0;

  return last[1] == 'c' && last[2] == '\0';
}

static char *clean_path(const char *root) {
  if (root == NULL)
    return NULL;

  char *clean = strdup(root);
  if (clean == NULL)
    return NULL;

  char *read = clean;
  char *write = clean;

  while (*read != '\0') {

    if (write > clean && write[-1] == '/' && *read == '/') {
      read++;
      continue;
    }

    *write++ = *read++;
  }

  if (write - clean > 1 && write[-1] == '/') {
    write--;
  }

  *write = '\0';

  return clean;
}

static int walk(const char *start, const IgnoreList *list, FileCallback cb,
                void *ctx) {
  DIR *dir = opendir(start);
  if (dir == NULL)
    return -1;

  struct dirent *entry;
  struct stat st;
  int result = 0;

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char slash[4096];
    int n = snprintf(slash, sizeof(slash), "%s/%s", start, entry->d_name);

    if (n < 0 || (size_t)n >= sizeof(slash))
      continue;

    if (lstat(slash, &st) == -1)
      continue;

    /* Only now do we know whether the entry is a directory. */
    if (ignore_match(list, entry->d_name, S_ISDIR(st.st_mode)))
      continue;

    if (S_ISDIR(st.st_mode)) {
      if (walk(slash, list, cb, ctx) == -1) {
        fprintf(stderr, "Failed to open dir: %s\n", slash);
        result = -1;
      }
    } else if (S_ISREG(st.st_mode)) {
      if (!ends_with_c(entry->d_name))
        continue;
      cb(slash, ctx);
    }
  }
  closedir(dir);
  return result;
}

int project_walk(const char *start, const char *root, FileCallback cb,
                 void *ctx) {
  if (start == NULL || cb == NULL)
    return -1;

  char *clean_start = clean_path(start);
  if (clean_start == NULL)
    return -1;

  IgnoreList list;
  if (ignore_load(root, &list) != 0) {
    free(clean_start);
    return -1;
  }

  int result = walk(clean_start, &list, cb, ctx);

  ignore_free(&list);
  free(clean_start);
  return result;
}

/* "/a/b/c" -> "/a/b"    "/a" -> "/" */
static void cut_last(char *dir) {
  char *slash = strrchr(dir, '/');
  if (slash == dir)
    dir[1] = '\0'; /* keep the leading '/' */
  else
    *slash = '\0';
}

char *project_find_root(const char *start) {
  char *dir = realpath(start, NULL);
  if (dir == NULL)
    return NULL;

  struct stat st;
  if (stat(dir, &st) != 0) {
    free(dir);
    return NULL;
  }
  if (!S_ISDIR(st.st_mode)) /* a file: start at its folder */
    cut_last(dir);

  char probe[4096];
  for (;;) {
    int n = snprintf(probe, sizeof(probe), "%s/%s", dir, HGENDIR);
    if (n < 0 || (size_t)n >= sizeof(probe)) {
      free(dir);
      return NULL;
    }
    if (stat(probe, &st) == 0 && S_ISDIR(st.st_mode))
      return dir;

    if (strcmp(dir, "/") == 0) { /* reached the top, nothing found */
      free(dir);
      return NULL;
    }
    cut_last(dir);
  }
}
