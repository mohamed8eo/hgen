#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "test.h"
#include "project/project.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

static const char *base = "build/test_project_tree";
static const char *notfound_base = "/tmp/opencode/test_project_notfound";

static char *abs_path(const char *rel) {
  return realpath(rel, NULL);
}

static void setup_tree(void) {
  /* Tree:
   * base/                    (no .hgen)
   * base/a/                  (has .hgen)
   * base/a/b/                (no .hgen -> finds base/a/.hgen)
   * base/a/b/c/              (no .hgen -> finds base/a/.hgen)
   * base/x/                  (walks up to repo root .hgen)
   */
  mkdir(base, 0755);
  mkdir("build/test_project_tree/a", 0755);
  mkdir("build/test_project_tree/a/b", 0755);
  mkdir("build/test_project_tree/a/b/c", 0755);
  mkdir("build/test_project_tree/x", 0755);

  /* Only base/a has a .hgen directory */
  mkdir("build/test_project_tree/a/.hgen", 0755);

  /* Not-found tree under /tmp (no .hgen anywhere above) */
  mkdir(notfound_base, 0755);
  mkdir("/tmp/opencode/test_project_notfound/sub", 0755);
}

static void teardown_tree(void) {
  rmdir("build/test_project_tree/a/b/c");
  rmdir("build/test_project_tree/a/b");
  rmdir("build/test_project_tree/a/.hgen");
  rmdir("build/test_project_tree/a");
  rmdir("build/test_project_tree/x");
  rmdir(base);

  rmdir("/tmp/opencode/test_project_notfound/sub");
  rmdir(notfound_base);
}

static void test_find_root_inside_a(void) {
  char *expected = abs_path("build/test_project_tree/a");
  char *root = project_find_root("build/test_project_tree/a/b");
  CHECK_STR(root, expected);
  free(root);
  free(expected);
}

static void test_find_root_deeper(void) {
  char *expected = abs_path("build/test_project_tree/a");
  char *root = project_find_root("build/test_project_tree/a/b/c");
  CHECK_STR(root, expected);
  free(root);
  free(expected);
}

static void test_find_root_at_a(void) {
  char *expected = abs_path("build/test_project_tree/a");
  char *root = project_find_root("build/test_project_tree/a");
  CHECK_STR(root, expected);
  free(root);
  free(expected);
}

static void test_find_root_not_found(void) {
  char *root = project_find_root("/tmp/opencode/test_project_notfound/sub");
  CHECK(root == NULL);
}

static void test_find_root_missing_path(void) {
  char *root = project_find_root("build/test_project_tree/does_not_exist");
  CHECK(root == NULL);
}

static void test_find_root_file_starts_at_parent(void) {
  /* A file path should start searching at its parent directory */
  FILE *fp = fopen("build/test_project_tree/a/b/notes.txt", "w");
  CHECK(fp != NULL);
  if (fp != NULL)
    fclose(fp);

  char *expected = abs_path("build/test_project_tree/a");
  char *root = project_find_root("build/test_project_tree/a/b/notes.txt");
  CHECK_STR(root, expected);
  free(root);
  free(expected);

  remove("build/test_project_tree/a/b/notes.txt");
}

int main(void) {
  setup_tree();

  test_find_root_inside_a();
  test_find_root_deeper();
  test_find_root_at_a();
  test_find_root_not_found();
  test_find_root_missing_path();
  test_find_root_file_starts_at_parent();

  teardown_tree();

  TEST_REPORT("project");
}
