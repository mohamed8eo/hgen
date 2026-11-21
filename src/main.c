#include <stdio.h>
#include <stdlib.h>

#include "fileio.h"
#include "scan.h"
#include "strip.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s file.c\n", argv[0]);
    return EXIT_FAILURE;
  }

  size_t out_len;
  char *buf = read_file(argv[1], &out_len);
  if (buf == NULL) {
    perror("read_file");
    return EXIT_FAILURE;
  }

  char *result = strip_comments_and_literals(buf);
  if (result == NULL) {
    perror("strip_comments_and_literals");
    free(buf);
    return EXIT_FAILURE;
  }
  scan(result);

  // printf("read %zu bytes\n", out_len);
  // printf("%s", result);

  free(buf);
  free(result);
  return EXIT_SUCCESS;
}
