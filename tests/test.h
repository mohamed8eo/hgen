#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Each test binary is a single translation unit, so these are file-local. */
static int test_checks;
static int test_failures;

#define CHECK(cond)                                                            \
  do {                                                                         \
    test_checks++;                                                             \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      test_failures++;                                                         \
    }                                                                          \
  } while (0)

#define CHECK_INT(got, want)                                                   \
  do {                                                                         \
    int tg_ = (int)(got);                                                      \
    int tw_ = (int)(want);                                                     \
    test_checks++;                                                             \
    if (tg_ != tw_) {                                                          \
      fprintf(stderr, "FAIL %s:%d: got %d, want %d\n", __FILE__, __LINE__,     \
              tg_, tw_);                                                       \
      test_failures++;                                                         \
    }                                                                          \
  } while (0)

#define CHECK_SIZE(got, want)                                                  \
  do {                                                                         \
    size_t tg_ = (size_t)(got);                                                \
    size_t tw_ = (size_t)(want);                                               \
    test_checks++;                                                             \
    if (tg_ != tw_) {                                                          \
      fprintf(stderr, "FAIL %s:%d: got %zu, want %zu\n", __FILE__, __LINE__,   \
              tg_, tw_);                                                       \
      test_failures++;                                                         \
    }                                                                          \
  } while (0)

#define CHECK_STR(got, want)                                                   \
  do {                                                                         \
    const char *tg_ = (got);                                                   \
    const char *tw_ = (want);                                                  \
    test_checks++;                                                             \
    if (strcmp(tg_, tw_) != 0) {                                               \
      fprintf(stderr, "FAIL %s:%d: got \"%s\", want \"%s\"\n", __FILE__,       \
              __LINE__, tg_, tw_);                                             \
      test_failures++;                                                         \
    }                                                                          \
  } while (0)

#define TEST_REPORT(name)                                                      \
  do {                                                                         \
    printf("%s: %d checks, %d failure(s)\n", (name), test_checks,              \
           test_failures);                                                     \
    return test_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;                   \
  } while (0)

#endif
