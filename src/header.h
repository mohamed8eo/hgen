#ifndef HEADER_H
#define HEADER_H

#include <stddef.h>

/* Returns a malloc'd .h path, or NULL on error. Caller must free it. */
char *header_path(const char *path, size_t path_len);

#endif
