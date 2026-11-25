#include "sync.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(FILE *stream, const char *prog) {
  fprintf(stream, "usage: %s [--verbose] [--dry-run] file.c\n", prog);
}

int main(int argc, char *argv[]) {
  const char *path = NULL;
  int verbose = 0;
  int dry_run = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--verbose") == 0) {
      verbose = 1;
    } else if (strcmp(argv[i], "--dry-run") == 0) {
      dry_run = 1;
    } else if (strcmp(argv[i], "--help") == 0) {
      usage(stdout, argv[0]);
      return EXIT_SUCCESS;
    } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
      usage(stderr, argv[0]);
      return EXIT_FAILURE;
    } else if (path == NULL) {
      path = argv[i];
    } else {
      usage(stderr, argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (path == NULL) {
    usage(stderr, argv[0]);
    return EXIT_FAILURE;
  }

  return sync_file(path, verbose, dry_run) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
