#include "cmd_init.h"
#include "project/ignore.h"
#define _DEFAULT_SOURCE

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../core/fileio.h"

/*
 * hgen init -- create a .hgen directory and a .hgenignore in the target.
 *
 * Usage:
 *   hgen init                 # initialize the current directory
 *   hgen init <path>          # initialize <path>
 *
 * .hgenignore format:
 *   # comment                 anything after '#' is a comment
 *   name                      skip that exact file or directory name
 *   dir/                      skip everything inside that directory
 *   *.ext                     skip every file ending in .ext
 *                             (the '*' matches any run of characters)
 *
 * Examples:
 *   build                     skip the build/ directory
 *   *.o                       skip every object file
 *   node_modules              skip the node_modules/ directory
 */

/*
 * default_ignore_text -- build the default .hgenignore contents.
 *
 * Returns a malloc'd string (caller frees it), or NULL on allocation failure.
 * The static list below covers things you usually do NOT want hgen to scan
 * or track: version control, build output, tests, dependencies, editor files,
 * OS junk, and core dumps.
 */
static char *default_ignore_text(void) {
  static const char *lines[] = {
      "# hgenignore -- files and directories hgen should skip",
      "#",
      "# Lines starting with '#' are comments.",
      "# A bare name skips that file or directory everywhere.",
      "# A name ending in '/' skips a whole directory.",
      "# A name containing '*' matches any run of characters (glob-style),",
      "#   e.g. '*.o' skips every object file, 'core.*' skips every core dump.",
      "#",
      "",
      "# --- Version control ---",
      ".git",
      ".gitignore",
      "",
      "# --- hgen itself ---",
      ".hgen",
      ".hgenignore",
      "",
      "# --- Build output ---",
      "build",
      "build/",
      "*.o",
      "*.a",
      "*.so",
      "core",
      "core.*",
      "",
      "# --- Tests and fixtures ---",
      "tests",
      "tests/",
      "fixtures",
      "expected",
      "",
      "# --- Dependencies ---",
      "node_modules",
      "",
      "# --- Editors / OS / backups ---",
      ".vscode",
      ".idea",
      ".DS_Store",
      "*~",
      "",
      NULL};

  size_t total = 0;
  for (size_t i = 0; lines[i] != NULL; i++)
    total += strlen(lines[i]) + 1; /* +1 for the '\n' */

  char *out = malloc(total + 1);
  if (out == NULL)
    return NULL;

  out[0] = '\0';
  for (size_t i = 0; lines[i] != NULL; i++) {
    strcat(out, lines[i]);
    strcat(out, "\n");
  }
  return out;
}

/*
 * write_hgenignore -- write the default .hgenignore into dir.
 * Returns 0 on success, -1 on failure.
 */
static int write_hgenignore(const char *dir) {
  char *path = NULL;
  char *text = NULL;
  int rc = -1;

  size_t n = snprintf(NULL, 0, "%s/%s", dir, HGENIGNORE) + 1;
  path = malloc(n);
  if (path == NULL)
    goto cleanup;
  snprintf(path, n, "%s/%s", dir, HGENIGNORE);

  /* Fix #2: never overwrite an existing .hgenignore. */
  if (access(path, F_OK) == 0) {
    fprintf(stderr, "hgen: %s already exists; leaving it alone\n", path);
    rc = 0;
    goto cleanup;
  }

  text = default_ignore_text();
  if (text == NULL)
    goto cleanup;

  if (write_file(path, text) != 0)
    goto cleanup;

  rc = 0;

cleanup:
  free(path);
  free(text);
  return rc;
}

int cmd_init(const char *target) {
  char cwd[PATH_MAX];
  char base[PATH_MAX];
  char hgen_dir[PATH_MAX];
  char hgenignore_path[PATH_MAX];
  const char *t = (target == NULL || target[0] == '\0') ? "." : target;

  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("getcwd");
    return EXIT_FAILURE;
  }

  /* Resolve target against cwd when it is relative. */
  if (t[0] == '/') {
    snprintf(base, sizeof(base), "%s", t);
  } else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(base, sizeof(base), "%s/%s", cwd, t);
#pragma GCC diagnostic pop
  }

  /* Create the hidden .hgen directory (0755 = rwxr-xr-x). Ignore EEXIST. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
  snprintf(hgen_dir, sizeof(hgen_dir), "%s/%s", base, HGENDIR);
#pragma GCC diagnostic pop
  if (mkdir(hgen_dir, 0755) == -1 && errno != EEXIST) {
    perror("mkdir");
    return EXIT_FAILURE;
  }

  /* Write .hgenignore only if it does not already exist. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
  snprintf(hgenignore_path, sizeof(hgenignore_path), "%s/%s", base, HGENIGNORE);
#pragma GCC diagnostic pop
  if (write_hgenignore(base) != 0) {
    perror("write .hgenignore");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
