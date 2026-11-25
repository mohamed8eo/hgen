#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "fileio.h"
#include "header.h"
#include "scan.h"
#include "source.h"
#include "strip.h"

int main(int argc, char *argv[]) {
  char *buf = NULL;
  char *result = NULL;
  char *old_text = NULL;
  char *hpath = NULL;
  FILE *existing = NULL;
  ItemList items = {0};
  Buf out = {0};
  int rc = EXIT_FAILURE;
  size_t out_len = 0;
  size_t old_len = 0;
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
    /* The file is there: keep its guard, includes and structs, and replace
       only the generated block between the markers. */
    fclose(existing);
    existing = NULL;

    old_text = read_file(hpath, &old_len);
    if (old_text == NULL)
      goto cleanup;
    if (header_update(old_text, &items, &out) != 0)
      goto cleanup;
    free(old_text);
    old_text = NULL;

    if (write_file(hpath, out.data) != 0)
      goto cleanup;
    fprintf(stderr, "Updated existing header %s\n", hpath);
  } else {
    if (header_build(result, hpath, &items, &out) != 0)
      goto cleanup;
    if (write_file(hpath, out.data) != 0)
      goto cleanup;
  }

  int added = 0;
  if (source_rewrite(buf, &items, hpath, path, &added) != 0)
    goto cleanup;
  if (added)
    fprintf(stderr, "Added #include of %s to %s\n", hpath, path);

  fputs(out.data, stdout);
  rc = EXIT_SUCCESS;

cleanup:
  if (existing != NULL)
    fclose(existing);
  free(old_text);
  buf_free(&out);
  itemlist_free(&items);
  free(hpath);
  free(result);
  free(buf);
  return rc;
}
