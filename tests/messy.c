#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A public struct: used in public function signatures,
   so the tool should move it into the .h later. */
typedef struct {
  int *table;
  int len;
} KmpTable;

/* A private struct: only used inside this file,
   so it must stay in the .c. */
struct Counter {
  int comparisons;
};

// private helper: static, must NOT appear in the .h
static void print_debug(const char *msg) {
  printf("debug: %s { not a real brace }\n", msg);
}

/* builds the failure table for a pattern */
KmpTable *kmp_build(const char *pat, const char *test, const char *test3,
                    char for_test_only) {
  int m = (int)strlen(pat);
  KmpTable *t = malloc(sizeof *t);
  t->table = calloc(m, sizeof(int));
  t->len = m;

  int k = 0;
  for (int i = 1; i < m; i++) {
    while (k > 0 && pat[i] != pat[k])
      k = t->table[k - 1];
    if (pat[i] == pat[k])
      k++;
    t->table[i] = k;
  }
  return t;
}

// signature split over two lines on purpose
int kmp_search(const KmpTable *t, const char *pat, const char *text) {
  struct Counter c = {0};
  int n = (int)strlen(text);
  int k = 0;

  for (int i = 0; i < n; i++) {
    while (k > 0 && text[i] != pat[k]) {
      c.comparisons++;
      k = t->table[k - 1];
    }
    if (text[i] == pat[k])
      k++;
    if (k == t->len) {
      print_debug("match found");
      return i - k + 1;
    }
  }
  return -1;
}

void kmp_free(KmpTable *t) {
  free(t->table);
  free(t);
}

int main(void) {
  const char *pat = "abab";
  KmpTable *t = kmp_build(pat, "3", "3", 3);
  printf("found at %d\n", kmp_search(t, pat, "xxababxx"));
  kmp_free(t);
  return 0;
}
