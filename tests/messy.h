#ifndef HGEN_MESSY_H
#define HGEN_MESSY_H

/* --- BEGIN PROTOTYPES (auto-generated, do not edit) --- */
KmpTable *kmp_build(const char *pat, const char *test, const char *test3, char for_test_only);
int kmp_search(const KmpTable *t, const char *pat, const char *text);
void kmp_free(KmpTable *t);
/* --- END PROTOTYPES --- */

#endif /* HGEN_MESSY_H */
