#define _DEFAULT_SOURCE
#include "test.h"
#include "project/ignore.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *tmpdir = "build/test_ignore_root";

static void setup_dir(void) {
  mkdir(tmpdir, 0755);
  char path[512];
  snprintf(path, sizeof(path), "%s/%s", tmpdir, HGENIGNORE);
  remove(path);
}

static void write_hgenignore(const char *text) {
  char path[512];
  snprintf(path, sizeof(path), "%s/%s", tmpdir, HGENIGNORE);
  FILE *fp = fopen(path, "w");
  CHECK(fp != NULL);
  if (fp != NULL) {
    fputs(text, fp);
    fclose(fp);
  }
}

/* Build a list directly (push_rule is static in ignore.c). */
static int build_list(IgnoreList *list, size_t count, ...) {
  va_list ap;
  va_start(ap, count);
  list->rules = malloc(count * sizeof(IgnoreRule));
  if (list->rules == NULL) {
    va_end(ap);
    return -1;
  }
  list->len = 0;
  for (size_t i = 0; i < count; i++) {
    const char *pat = va_arg(ap, const char *);
    int dir_only = va_arg(ap, int);
    list->rules[i].pat = strdup(pat);
    if (list->rules[i].pat == NULL) {
      va_end(ap);
      return -1;
    }
    list->rules[i].dir_only = dir_only;
    list->len++;
  }
  va_end(ap);
  return 0;
}

/* ---- ignore_match unit tests ---- */

static void test_match_basic(void) {
  IgnoreList list = {NULL, 0};
  CHECK_INT(build_list(&list, 2, "*.o", 0, "build", 1), 0);

  CHECK_INT(ignore_match(&list, "a.o", 0), 1);
  CHECK_INT(ignore_match(&list, "a.c", 0), 0);
  CHECK_INT(ignore_match(&list, "build", 1), 1);
  CHECK_INT(ignore_match(&list, "build", 0), 0);
  CHECK_INT(ignore_match(&list, "main.c", 0), 0);

  ignore_free(&list);
}

static void test_match_star(void) {
  IgnoreList list = {NULL, 0};
  CHECK_INT(build_list(&list, 1, "*", 0), 0);

  CHECK_INT(ignore_match(&list, "anything.c", 0), 1);
  CHECK_INT(ignore_match(&list, ".hidden", 0), 1);
  CHECK_INT(ignore_match(&list, "", 0), 1);

  ignore_free(&list);
}

static void test_match_core_glob(void) {
  IgnoreList list = {NULL, 0};
  CHECK_INT(build_list(&list, 1, "core.*", 0), 0);

  CHECK_INT(ignore_match(&list, "core.123", 0), 1);
  CHECK_INT(ignore_match(&list, "core.", 0), 1);
  CHECK_INT(ignore_match(&list, "core", 0), 0);
  CHECK_INT(ignore_match(&list, "notcore.1", 0), 0);

  ignore_free(&list);
}

static void test_match_empty_list(void) {
  IgnoreList list = {NULL, 0};
  CHECK_INT(ignore_match(&list, "anything.c", 0), 0);
}

/* ---- ignore_load tests (file-based) ---- */

static void test_load_missing_file_defaults(void) {
  setup_dir();
  IgnoreList list = {NULL, 0};
  CHECK_INT(ignore_load(tmpdir, &list), 0);

  /* universal defaults: .git, .hgen, build, node_modules */
  CHECK_SIZE(list.len, 4);
  if (list.len >= 4) {
    CHECK_STR(list.rules[0].pat, ".git");
    CHECK_INT(list.rules[0].dir_only, 0);
    CHECK_STR(list.rules[1].pat, ".hgen");
    CHECK_STR(list.rules[2].pat, "build");
    CHECK_STR(list.rules[3].pat, "node_modules");
  }

  /* defaults behave like normal rules ("build" has no trailing /) */
  CHECK_INT(ignore_match(&list, "build", 1), 1);
  CHECK_INT(ignore_match(&list, "build", 0), 1);
  CHECK_INT(ignore_match(&list, "node_modules", 1), 1);
  CHECK_INT(ignore_match(&list, "node_modules", 0), 1);

  /* No .hgenignore or .hgen was written into tmpdir */
  char probe[512];
  snprintf(probe, sizeof(probe), "%s/%s", tmpdir, HGENIGNORE);
  CHECK(access(probe, F_OK) != 0);
  snprintf(probe, sizeof(probe), "%s/%s", tmpdir, HGENDIR);
  CHECK(access(probe, F_OK) != 0);

  ignore_free(&list);
}

static void test_load_null_root_defaults(void) {
  IgnoreList list = {NULL, 0};
  CHECK_INT(ignore_load(NULL, &list), 0);
  CHECK_SIZE(list.len, 4);
  if (list.len >= 4) {
    CHECK_STR(list.rules[0].pat, ".git");
    CHECK_STR(list.rules[1].pat, ".hgen");
    CHECK_STR(list.rules[2].pat, "build");
    CHECK_STR(list.rules[3].pat, "node_modules");
  }
  ignore_free(&list);
}

static void test_load_parses_file(void) {
  setup_dir();
  write_hgenignore("# build output\nbuild/\n*.o\n\nnode_modules\n");
  IgnoreList list = {NULL, 0};
  CHECK_INT(ignore_load(tmpdir, &list), 0);

  CHECK_SIZE(list.len, 3);
  if (list.len >= 3) {
    CHECK_STR(list.rules[0].pat, "build");
    CHECK_INT(list.rules[0].dir_only, 1);
    CHECK_STR(list.rules[1].pat, "*.o");
    CHECK_INT(list.rules[1].dir_only, 0);
    CHECK_STR(list.rules[2].pat, "node_modules");
    CHECK_INT(list.rules[2].dir_only, 0);
  }

  CHECK_INT(ignore_match(&list, "build", 1), 1);
  CHECK_INT(ignore_match(&list, "build", 0), 0);
  CHECK_INT(ignore_match(&list, "a.o", 0), 1);
  CHECK_INT(ignore_match(&list, "a.c", 0), 0);
  CHECK_INT(ignore_match(&list, "node_modules", 1), 1);
  CHECK_INT(ignore_match(&list, "node_modules", 0), 1);

  ignore_free(&list);
}

static void test_load_comments_and_blanks_ignored(void) {
  setup_dir();
  write_hgenignore("\n   \n# comment\n#another\n\n");
  IgnoreList list = {NULL, 0};
  CHECK_INT(ignore_load(tmpdir, &list), 0);
  CHECK_SIZE(list.len, 0);
  ignore_free(&list);
}

static void test_load_trailing_crlf_trimmed(void) {
  setup_dir();
  write_hgenignore("*.o\r\nbuild/\r\n");
  IgnoreList list = {NULL, 0};
  CHECK_INT(ignore_load(tmpdir, &list), 0);
  CHECK_SIZE(list.len, 2);
  if (list.len >= 2) {
    CHECK_STR(list.rules[0].pat, "*.o");
    CHECK_STR(list.rules[1].pat, "build");
    CHECK_INT(list.rules[1].dir_only, 1);
  }
  ignore_free(&list);
}

static void test_free_resets(void) {
  IgnoreList list = {NULL, 0};
  CHECK_INT(build_list(&list, 1, "*.o", 0), 0);
  CHECK_SIZE(list.len, 1);
  ignore_free(&list);
  CHECK_INT(list.len, 0);
  CHECK(list.rules == NULL);
}

int main(void) {
  test_match_basic();
  test_match_star();
  test_match_core_glob();
  test_match_empty_list();
  test_load_missing_file_defaults();
  test_load_null_root_defaults();
  test_load_parses_file();
  test_load_comments_and_blanks_ignored();
  test_load_trailing_crlf_trimmed();
  test_free_resets();

  char path[512];
  snprintf(path, sizeof(path), "%s/%s", tmpdir, HGENIGNORE);
  remove(path);
  rmdir(tmpdir);

  TEST_REPORT("ignore");
}
