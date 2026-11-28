#define _POSIX_C_SOURCE 200809L

#include "project.h"
#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *IGNORED[] = {".git", ".hgen", "build", "node_modules"};
#define IGNORE_COUNT (sizeof(IGNORED) / sizeof(IGNORED[0]))

static int should_ignore(const char *name) {
  for (size_t i = 0; i < IGNORE_COUNT; i++) {
    if (strcmp(name, IGNORED[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

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

int project_walk(const char *root, FileCallback cb, void *ctx) {
  if (root == NULL || cb == NULL)
    return -1;

  char *clean_root = clean_path(root);
  if (clean_root == NULL)
    return -1;

  DIR *dir = opendir(clean_root);
  if (dir == NULL) {
    free(clean_root);
    return -1;
  }

  struct dirent *entry;
  struct stat st;
  int result = 0;

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    if (should_ignore(entry->d_name))
      continue;

    char path[4096];
    int n = snprintf(path, sizeof(path), "%s/%s", clean_root, entry->d_name);

    if (n < 0 || (size_t)n >= sizeof(path)) {
      continue;
    }

    if (lstat(path, &st) == -1) {
      continue;
    }

    if (S_ISDIR(st.st_mode)) {
      // printf("Directory: %s\n", entry->d_name);
      if (project_walk(path, cb, ctx) == -1) {
        fprintf(stderr, "Failed to open dir: %s\n", path);
        result = -1;
      }
    } else if (S_ISREG(st.st_mode)) {
      // printf("File : %s\n", entry->d_name);
      if (!ends_with_c(entry->d_name))
        continue;
      cb(path, ctx);
    }
  }
  closedir(dir);
  free(clean_root);
  return result;
}
