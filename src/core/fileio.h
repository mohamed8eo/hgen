#ifndef FILEIO_H
#define FILEIO_H

#include <stddef.h>

char *read_file(const char *path, size_t *out_len);
int write_file(const char *path, const char *text);

#endif
