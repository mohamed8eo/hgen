#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "fileio.h"
#include "header.h"
#include "scan.h"
#include "strip.h"

int main(int argc, char *argv[]) {
  char *buf = NULL;
  char *result = NULL;
  char *hpath = NULL;
  FILE *existing = NULL;
  ItemList items = {0};
  Buf out = {0};
  int rc = EXIT_FAILURE;
  size_t out_len = 0;
  size_t path_len = 0;
  const char *path = NULL;

  if (argc < 2) {
    fprintf(stderr, "usage: %s file.c\n", argv[0]);
    goto cleanup;
  }

  path = argv[1];
  path_len = strlen(path);

  buf = read_file(path, &out_len);
  if (buf == NULL)
    goto cleanup;

  result = strip_comments_and_literals(buf);
  if (result == NULL)
    goto cleanup;

  scan(result, &items);

  hpath = header_path(path, path_len);
  if (hpath == NULL)
    goto cleanup;

  existing = fopen(hpath, "r");
  if (existing != NULL) {
    fprintf(stderr, "Header %s already exists; refusing to overwrite\n", hpath);
    rc = EXIT_SUCCESS;
    goto cleanup;
  }

  if (header_build(result, hpath, &items, &out) != 0)
    goto cleanup;
  if (write_file(hpath, out.data) != 0)
    goto cleanup;
  if (header_move_structs(buf, &items, hpath, path) != 0)
    goto cleanup;

  fputs(out.data, stdout);
  rc = EXIT_SUCCESS;

cleanup:
  if (existing != NULL)
    fclose(existing);
  buf_free(&out);
  itemlist_free(&items);
  free(hpath);
  free(result);
  free(buf);
  return rc;
}
