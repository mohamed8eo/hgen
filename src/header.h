#ifndef HEADER_H
#define HEADER_H

#include "buf.h"
#include "scan.h"
#include <stddef.h>

/* Returns a malloc'd .h path, or NULL on error. Caller must free it. */
char *header_path(const char *path, size_t path_len);
int header_build(const char *src, const char *h_path, const ItemList *list,
                 Buf *out);
/* Move the structs emitted into the header out of the source file, adding an
   #include of the header when the source does not already have one. */
int header_move_structs(const char *src, const ItemList *list,
                        const char *h_path, const char *c_path);
int write_file(const char *path, const char *text);

#endif
