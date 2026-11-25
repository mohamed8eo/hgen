#ifndef HEADER_H
#define HEADER_H

#include "buf.h"
#include "scan.h"

#include <stddef.h>

/* Returns a malloc'd .h path, or NULL on error. Caller must free it. */
char *header_path(const char *path, size_t path_len);
int header_build(const char *src, const char *h_path, const ItemList *list,
                 Buf *out);
int header_update(const char *old_text, const ItemList *list, Buf *out);

#endif /* HEADER_H */
