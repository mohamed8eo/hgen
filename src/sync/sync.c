#include "sync.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/buf.h"
#include "core/fileio.h"
#include "pipeline/header.h"
#include "scan/scan.h"
#include "pipeline/source.h"
#include "core/strip.h"
#include "pipeline/structs.h"

/* Whitespace-insensitive copies, used to spot structs that never made it
   into an existing header despite formatting differences. */
static char *without_ws(const char *s, size_t start, size_t end) {
  char *out = malloc(end - start + 1);
  if (out == NULL)
    return NULL;

  size_t n = 0;
  for (size_t i = start; i < end; i++) {
    if (!isspace((unsigned char)s[i]))
      out[n++] = s[i];
  }
  out[n] = '\0';
  return out;
}

static char *one_line(const char *s, size_t start, size_t end) {
  char *out = malloc(end - start + 1);
  if (out == NULL)
    return NULL;

  size_t n = 0;
  int pending_space = 0;
  for (size_t i = start; i < end; i++) {
    if (isspace((unsigned char)s[i])) {
      pending_space = 1;
    } else {
      if (n > 0 && pending_space)
        out[n++] = ' ';
      pending_space = 0;
      out[n++] = s[i];
    }
  }
  out[n] = '\0';
  return out;
}

/* header_update only replaces the prototype block, so a struct added to the
   .c after the .h was created stays in the .c. Warn until that is fixed. */
static void warn_missing_structs(const char *src, const ItemList *items,
                                 const char *header, const char *c_path,
                                 const char *hpath) {
  unsigned char *flags = struct_flags(src, items, NULL);
  if (flags == NULL)
    return;

  char *flat_header = without_ws(header, 0, strlen(header));
  for (size_t i = 0; i < items->len; i++) {
    const Item *ite = &items->items[i];
    if (ite->kind != ITEM_STRUCT || !flags[i])
      continue;

    char *flat = without_ws(src, ite->start, ite->end);
    if (flat == NULL)
      continue;

    if (flat_header == NULL || strstr(flat_header, flat) == NULL) {
      char *label = one_line(src, ite->start, ite->end);
      fprintf(stderr,
              "warning: public struct in %s is missing from %s; "
              "header_update does not copy new structs yet: %s\n",
              c_path, hpath, label != NULL ? label : "(struct)");
      free(label);
    }
    free(flat);
  }

  free(flat_header);
  free(flags);
}

/* A file with no exportable function prototypes (e.g. main.c, or a file of
   only static helpers) has nothing a header can usefully declare. */
static int has_exportable_items(const ItemList *items) {
  for (size_t i = 0; i < items->len; i++) {
    if (items->items[i].kind == ITEM_FUNC)
      return 1;
  }
  return 0;
}

static void report(const char *c_path, const char *hpath, int created,
                   int updated, int source_changed, int dry_run) {
  fprintf(stderr, "hgen: %s: ", c_path);

  if (created)
    fprintf(stderr, "%s %s", dry_run ? "would create" : "created", hpath);
  else if (updated)
    fprintf(stderr, "%s %s", dry_run ? "would update" : "updated", hpath);
  else
    fputs("header unchanged", stderr);

  if (created) {
    if (source_changed)
      fputs(dry_run ? ", would rewrite source" : ", source updated", stderr);
    else
      fputs(", source unchanged", stderr);
  }

  fputc('\n', stderr);
}

SyncResult sync_file(const char *c_path, int verbose, int dry_run) {
  char *buf = NULL;
  char *result = NULL;
  char *old_text = NULL;
  char *source_preview = NULL;
  char *hpath = NULL;
  FILE *existing = NULL;
  ItemList items = {0};
  Buf out = {0};
  SyncResult rc = SYNC_ERROR;
  int created = 0;
  int updated = 0;
  int source_changed = 0;
  size_t out_len = 0;
  size_t old_len = 0;
  size_t path_len = 0;

  if (c_path == NULL)
    return SYNC_ERROR;
  path_len = strlen(c_path);

  buf = read_file(c_path, &out_len);
  if (buf == NULL)
    goto cleanup;

  result = strip_comments_and_literals(buf);
  if (result == NULL)
    goto cleanup;

  scan(result, &items);

  if (!has_exportable_items(&items)) {
    rc = SYNC_SKIPPED;
    goto cleanup;
  }

  hpath = header_path(c_path, path_len);
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

    warn_missing_structs(result, &items, old_text, c_path, hpath);
    if (header_update(old_text, &items, &out) != 0)
      goto cleanup;

    if (strcmp(old_text, out.data) != 0) {
      updated = 1;
      if (!dry_run && write_file(hpath, out.data) != 0)
        goto cleanup;
    }
  } else {
    created = 1;
    if (header_build(result, hpath, &items, &out) != 0)
      goto cleanup;
    if (!dry_run && write_file(hpath, out.data) != 0)
      goto cleanup;
    if (source_rewrite(buf, &items, hpath, c_path, &source_changed, dry_run,
                       &source_preview) != 0)
      goto cleanup;
  }

  report(c_path, hpath, created, updated, source_changed, dry_run);

  if (dry_run) {
    if (out.data != NULL) {
      printf("--- would write %s ---\n", hpath);
      fputs(out.data, stdout);
    }
    if (source_preview != NULL) {
      printf("--- would rewrite %s ---\n", c_path);
      fputs(source_preview, stdout);
    }
  } else if (verbose && out.data != NULL) {
    fputs(out.data, stdout);
  }

  if (created)
    rc = SYNC_CREATED;
  else if (updated)
    rc = SYNC_UPDATED;
  else
    rc = SYNC_UNCHANGED;

cleanup:
  if (existing != NULL)
    fclose(existing);
  free(old_text);
  free(source_preview);
  buf_free(&out);
  itemlist_free(&items);
  free(hpath);
  free(result);
  free(buf);
  return rc;
}
