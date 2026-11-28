#define _DEFAULT_SOURCE

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int cmd_init(void) {
  char cwd[PATH_MAX];

  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("getcwd");
    return EXIT_FAILURE;
  }

  char dir_path[PATH_MAX];
  char file_path[PATH_MAX];

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
  snprintf(dir_path, sizeof(dir_path), "%s/.hgen", cwd);
  snprintf(file_path, sizeof(file_path), "%s/.hgenignore", cwd);
#pragma GCC diagnostic pop
  // Create hidden folder (0755 = rwxr-xr-x)
  if (mkdir(dir_path, 0755) == -1 && errno != EEXIST) {
    perror("mkdir");
    return EXIT_FAILURE;
  }

  FILE *fp = fopen(file_path, "w");
  if (fp == NULL) {
    perror("fopen");
    return EXIT_FAILURE;
  }

  fprintf(fp, "tests\n");
  fprintf(fp, "build\n");
  fclose(fp);

  return EXIT_SUCCESS;
}
