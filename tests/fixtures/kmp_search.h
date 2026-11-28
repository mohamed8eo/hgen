#ifndef HGEN_KMP_SEARCH_H
#define HGEN_KMP_SEARCH_H
#define MAX 10

typedef struct {
  int *table;
  int len;
} KmpTable;

/* --- BEGIN PROTOTYPES (auto-generated, do not edit) --- */
KmpTable *kmp_build(const char *pat, const char *test, char fuck, int too);
int kmp_search(const KmpTable *t, const char *pat, const char *text);
void kmp_free(KmpTable *t);
/* --- END PROTOTYPES --- */

#endif /* HGEN_KMP_SEARCH_H */
