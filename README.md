# hgen

A small, work-in-progress C header generator. Given a C source file, `hgen`
strips comments and string/char literals, finds the top-level declarations, and
generates a matching `.h` file containing the public function prototypes and
the struct declarations those prototypes use. Those structs are then removed
from the `.c` file, which gains an `#include` of its new header.

## Current state

Working:

- Reading a `.c` file from disk.
- Stripping `//` and `/* ... */` comments, plus the contents of string and char
  literals (newlines are kept so line positions are preserved).
- Skipping preprocessor directives, tracking brace depth to find top-level
  declarations, and splitting declarations on `;`.
- Generating a header with public function prototypes and the struct
  declarations they use (private structs stay in the `.c`), and writing it to
  disk if it does not already exist.
- Moving those structs out of the `.c` file and adding `#include "file.h"`
  there, so the source still compiles.
- Printing the generated header to stdout.

Not implemented yet:

- Replacing the contents of the generated block on an existing header.

## Building

Requires a C11 compiler and `make`.

```sh
make            # builds build/hgen
make clean      # removes the build directory
```

Compilation uses `-Wall -Wextra -Werror -std=c11`. `CC`, `CFLAGS`, `CPPFLAGS`,
and `LDFLAGS` can be overridden in the usual way.

## Usage

```sh
./build/hgen file.c
```

For example, running it on the included fixture:

```console
$ ./build/hgen tests/kmp_search.c
#ifndef HGEN_KMP_SEARCH_H
#define HGEN_KMP_SEARCH_H

/* --- BEGIN PROTOTYPES (auto-generated, do not edit) --- */
typedef struct {
  int *table;
  int len;
} KmpTable;

KmpTable *kmp_build(const char *pat);
int kmp_search(const KmpTable *t, const char *pat, const char *text);
void kmp_free(KmpTable *t);
/* --- END PROTOTYPES --- */

#endif /* HGEN_KMP_SEARCH_H */
```

The same text is written to `tests/kmp_search.h`, and the public `KmpTable`
definition is removed from `tests/kmp_search.c` (which gains
`#include "kmp_search.h"`). If the header already exists, the tool refuses to
overwrite it and prints a message to stderr; the source is left untouched.

The include guard is derived from the header's basename with a deliberate
`HGEN_` prefix: `tests/kmp_search.c` produces `HGEN_KMP_SEARCH_H`. The prefix
avoids collisions with user-written guards and keeps the guard a valid C
identifier even when the filename starts with a digit (`1counter.c` produces
`HGEN_1COUNTER_H`).

## Project layout

- `src/main.c` — CLI entry point; reads the file, strips it, scans it, then
  writes the header and rewrites the source.
- `src/fileio.c` — reads a whole file into a NUL-terminated buffer.
- `src/strip.c` — comment/literal stripping state machine.
- `src/scan.c` — top-level declaration scanner.
- `src/header.c`, `src/buf.c` — header text builder and growable output buffer.
- `tests/kmp_search.c`, `tests/messy.c` — sample inputs with comments,
  literals, braces inside strings, and a signature split over two lines.

## Known limitations

- Only one input file per invocation.
- An existing header is never updated; the tool refuses to overwrite it and
  does not move structs out of the source in that case.
- No special handling yet for multi-line macros, complex function pointers, or
  declarations spanning preprocessor conditionals.
- Structs using `size_t` gain an automatic `#include <stddef.h>`; other types
  needed by an emitted struct must be supplied by the includer.
