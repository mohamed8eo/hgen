#ifndef STRUCTS_H
#define STRUCTS_H

#include "scan/scan.h"

#include <stddef.h>

/* One flag per item: true for structs that belong in the header.
   Sets *need_stddef when an emitted struct uses size_t. Caller frees. */
unsigned char *struct_flags(const char *src, const ItemList *list,
                            int *need_stddef);

#endif /* STRUCTS_H */
