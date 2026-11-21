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

Not implemented yet:

- Writing an actual header file. `src/header.c` and `src/buf.c` are currently
  empty placeholders.
- Telling public declarations from private ones: `static` helpers and
  file-local structs are printed today just like public functions.
- An automated test suite. `tests/` contains sample inputs, not runnable tests.

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
typedef struct 
struct Counter 
static void print_debug(const char *msg) 
KmpTable *kmp_build(const char *pat) 
int kmp_search(const KmpTable *t, const char *pat, const char *text) 
void kmp_free(KmpTable *t) 
int main(void) 
```

## Project layout

- `src/main.c` — CLI entry point; reads the file, strips it, scans it.
- `src/fileio.c` — reads a whole file into a NUL-terminated buffer.
- `src/strip.c` — comment/literal stripping state machine.
- `src/scan.c` — top-level declaration scanner; prints to stdout.
- `src/header.c`, `src/buf.c` — placeholders for future header generation.
- `tests/kmp_search.c`, `tests/messy.c` — sample inputs with comments,
  literals, braces inside strings, and a signature split over two lines.

## Known limitations

- Only one input file per invocation, and output goes to stdout; no `.h` is
  written.
- `static` functions, private structs, and `main` are all included in the
  output.
- No special handling yet for multi-line macros, complex function pointers, or
  declarations spanning preprocessor conditionals.
