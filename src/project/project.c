#define _POSIX_C_SOURCE 200809L

#include "project.h"
#include "project/ignore.h"
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

static int walk(const char *root, const IgnoreList *list, FileCallback cb,
                void *ctx) {
  DIR *dir = opendir(root);
  if (dir == NULL)
    return -1;

  struct dirent *entry;
  struct stat st;
  int result = 0;

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char path[4096];
    int n = snprintf(path, sizeof(path), "%s/%s", root, entry->d_name);

    if (n < 0 || (size_t)n >= sizeof(path))
      continue;

    if (lstat(path, &st) == -1)
      continue;

    /* Only now do we know whether the entry is a directory. */
    if (ignore_match(list, entry->d_name, S_ISDIR(st.st_mode)))
      continue;

    if (S_ISDIR(st.st_mode)) {
      if (walk(path, list, cb, ctx) == -1) {
        fprintf(stderr, "Failed to open dir: %s\n", path);
        result = -1;
      }
    } else if (S_ISREG(st.st_mode)) {
      if (!ends_with_c(entry->d_name))
        continue;
      cb(path, ctx);
    }
  }
  closedir(dir);
  return result;
}

int project_walk(const char *root, FileCallback cb, void *ctx) {
  if (root == NULL || cb == NULL)
    return -1;

  char *clean_root = clean_path(root);
  if (clean_root == NULL)
    return -1;

  IgnoreList list;
  if (ignore_load(clean_root, &list) != 0) {
    free(clean_root);
    return -1;
  }

  int result = walk(clean_root, &list, cb, ctx);

  ignore_free(&list);
  free(clean_root);
  return result;
}
