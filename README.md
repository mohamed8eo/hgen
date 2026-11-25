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
  declarations they use (private structs stay in the `.c`).
- Creating a missing header, or updating only the generated block of an
  existing one while keeping its guard, includes and structs.
- Moving those structs out of the `.c` file.
- Checking whether the `.c` already includes its `.h`; adding
  `#include "file.h"` only when it is missing, otherwise skipping it.
- Printing one status line per file; dumping the generated header to stdout
  with `--verbose`, or previewing all writes with `--dry-run`.

## Building

Requires a C11 compiler and `make`.

```sh
make            # builds build/hgen
make test       # builds and runs all tests, including golden-header diffs
make test-sanitize  # same, with AddressSanitizer and UBSan
make clean      # removes the build directory
```

Compilation uses `-Wall -Wextra -Werror -std=c11`. `CC`, `CFLAGS`, `CPPFLAGS`,
and `LDFLAGS` can be overridden in the usual way. `make test` runs the CLI on
disposable copies of the fixtures, so tracked sources are never modified.

## Usage

```sh
./build/hgen file.c                # one status line on stderr
./build/hgen --verbose file.c      # also dump the generated header to stdout
./build/hgen --dry-run file.c      # preview header and source writes; change nothing
```

For example, running it on the included fixture:

```console
$ ./build/hgen tests/kmp_search.c
#ifndef HGEN_KMP_SEARCH_H
#define HGEN_KMP_SEARCH_H

typedef struct {
  int *table;
  int len;
} KmpTable;

/* --- BEGIN PROTOTYPES (auto-generated, do not edit) --- */
KmpTable *kmp_build(const char *pat);
int kmp_search(const KmpTable *t, const char *pat, const char *text);
void kmp_free(KmpTable *t);
/* --- END PROTOTYPES --- */

#endif /* HGEN_KMP_SEARCH_H */
```

The same text is written to `tests/kmp_search.h`, and the public `KmpTable`
definition is removed from `tests/kmp_search.c`. The source is then checked for
an include of its header: `#include "kmp_search.h"` is added only if it is not
already there, otherwise nothing is inserted.

If the header already exists, it is not regenerated from scratch. Instead,
everything between the BEGIN and END markers is replaced with the current
prototypes, while the include guard, includes and struct definitions above the
markers (and the `#endif` below them) are preserved. Any duplicate END markers
left by older runs are collapsed into one. The source is updated the same way
as for a new header.

The include guard is derived from the header's basename with a deliberate
`HGEN_` prefix: `tests/kmp_search.c` produces `HGEN_KMP_SEARCH_H`. The prefix
avoids collisions with user-written guards and keeps the guard a valid C
identifier even when the filename starts with a digit (`1counter.c` produces
`HGEN_1COUNTER_H`).

## Project layout

- `src/main.c` — CLI entry point; reads the file, strips it, scans it, then
  writes the header and rewrites the source.
- `src/fileio.c` — reads and writes whole files.
- `src/strip.c` — comment/literal stripping state machine.
- `src/scan.c` — top-level declaration scanner.
- `src/structs.c` — decides which structs belong in the header.
- `src/header.c` — guard generation, header text rendering and in-place updates.
- `src/source.c` — struct removal and include detection/insertion in the `.c`.
- `src/buf.c` — growable output buffer.
- `tests/kmp_search.c`, `tests/messy.c` — sample inputs with comments,
  literals, braces inside strings, and a signature split over two lines.

## Known limitations

- Only one input file per invocation.
- Updating an existing header only refreshes the prototype block: newly added
  public structs are not copied into a header that already exists.
- No special handling yet for multi-line macros, complex function pointers, or
  declarations spanning preprocessor conditionals.
- Structs using `size_t` gain an automatic `#include <stddef.h>`; other types
  needed by an emitted struct must be supplied by the includer.
- Public/private detection is a heuristic: a struct is considered public when
  its tag or typedef alias appears in a prototype. A name used coincidentally
  in both a private struct and a public signature may be moved by mistake.
