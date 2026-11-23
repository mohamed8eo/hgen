#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "fileio.h"
#include "header.h"
#include "scan.h"
#include "strip.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s file.c\n", argv[0]);
    return EXIT_FAILURE;
  }

  size_t out_len;
  const char *path = argv[1];
  size_t path_len = strlen(path);
  char *buf = read_file(path, &out_len);
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

  ItemList items = {0};
  scan(result, &items);

  char *hpath = header_path(path, path_len);
  if (hpath == NULL) {
    fprintf(stderr, "not a .c file\n");
    itemlist_free(&items);
    free(result);
    free(buf);
    return EXIT_FAILURE;
  }

  FILE *existing = fopen(hpath, "r");
  if (existing != NULL) {
    fclose(existing);
    fprintf(stderr, "Header %s already exists; refusing to overwrite\n", hpath);
    free(hpath);
    itemlist_free(&items);
    free(result);
    free(buf);
    return EXIT_SUCCESS;
  }

  Buf out = {0};
  if (header_build(result, hpath, &items, &out) != 0) {
    fprintf(stderr, "header_build failed\n");
    goto fail;
  }
  if (write_file(hpath, out.data) != 0) {
    fprintf(stderr, "write_file failed\n");
    goto fail;
  }

  fputs(out.data, stdout);

  buf_free(&out);
  itemlist_free(&items);
  free(hpath);
  free(result);
  free(buf);
  return EXIT_SUCCESS;

fail:
  buf_free(&out);
  itemlist_free(&items);
  free(hpath);
  free(result);
  free(buf);
  return EXIT_FAILURE;
}
