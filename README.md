# hgen

A small, work-in-progress C header generator. Given a C source file, `hgen`
strips comments and string/char literals, finds the top-level declarations, and
prints them to stdout. The intended end goal is to generate a matching `.h`
file from them.

## Current state

Working:

- Reading a `.c` file from disk.
- Stripping `//` and `/* ... */` comments, plus the contents of string and char
  literals (newlines are kept so line positions are preserved).
- Skipping preprocessor directives, tracking brace depth to find top-level
  declarations, and splitting declarations on `;`.
- Generating a header containing only public function prototypes, and writing
  it to disk if it does not already exist.
- Printing the generated header to stdout.

Not implemented yet:

- Telling public structs from private ones: no structs are written to the
  header yet, even when a public prototype uses one.
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
KmpTable *kmp_build(const char *pat);
int kmp_search(const KmpTable *t, const char *pat, const char *text);
void kmp_free(KmpTable *t);
/* --- END PROTOTYPES --- */

#endif /* HGEN_KMP_SEARCH_H */
```

The same text is written to `tests/kmp_search.h`. If that file already exists,
the tool refuses to overwrite it and prints a message to stderr.

## Project layout

- `src/main.c` — CLI entry point; reads the file, strips it, scans it.
- `src/fileio.c` — reads a whole file into a NUL-terminated buffer.
- `src/strip.c` — comment/literal stripping state machine.
- `src/scan.c` — top-level declaration scanner.
- `src/header.c`, `src/buf.c` — header text builder and growable output buffer.
- `tests/kmp_search.c`, `tests/messy.c` — sample inputs with comments,
  literals, braces inside strings, and a signature split over two lines.

## Known limitations

- Only one input file per invocation, and output goes to stdout; no `.h` is
  written.
- `static` functions, private structs, and `main` are all included in the
  output.
- No special handling yet for multi-line macros, complex function pointers, or
  declarations spanning preprocessor conditionals.
