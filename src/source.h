#ifndef SOURCE_H
#define SOURCE_H

#include "scan.h"

/* 1 = src includes its own header, 0 = it does not, -1 = bad input. */
int source_has_own_include(const char *src, const char *h_path);

/* Rewrite the .c exactly once: remove the structs emitted into the header,
   then add the header include only when it is missing. Reports via *added. */
int source_rewrite(const char *src, const ItemList *list,
                   const char *h_path, const char *c_path, int *added);

/* Check and add the include on an existing .c file. Reports via *added. */
int source_ensure_include(const char *c_path, const char *h_path, int *added);

#endif /* SOURCE_H */
